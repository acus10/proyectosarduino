#include <OneWire.h>
#include <TM1637Display.h>
#define RELE_ON 0
#define RELE_OFF 1
#define CLK 7
#define DIO 6

TM1637Display display(CLK, DIO);

const uint8_t celsiusSymbol[] = {
  SEG_A | SEG_B | SEG_G | SEG_F, //GRADOS
  SEG_A | SEG_F | SEG_E | SEG_D // C
};
// OneWire DS18S20, DS18B20, DS1822 Temperature Example
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// https://github.com/milesburton/Arduino-Temperature-Control-Library

OneWire  dsGabinete(2);  // on pin 2 (a 4.7K resistor is necessary)
//OneWire  dsDisipador(3);  // on pin 3 (a 4.7K resistor is necessary)
byte rele_fans = 3;
byte rele_peltier = 4; // on pin 4
float temp_corte = 6.4;
float temp_min = 5.0;
float histeresis = 1.6;

void setup() {
  display.setBrightness(5);
  pinMode(rele_peltier, OUTPUT);  
  pinMode(rele_fans, OUTPUT); 
  digitalWrite(rele_peltier, RELE_OFF);
  digitalWrite(rele_fans, RELE_OFF); 
  
}

void loop(void) {
  float temp = 0.0; 
  int tempAct = 0;  
  int tempAnt = 0;
  while (temp < temp_corte + histeresis){
    temp = medirTemperaturaGabinete();
    if  (temp > temp_min) {  
      tempAnt = tempAct;   
      tempAct = temp;      
      if (tempAct != tempAnt){
        display.showNumberDec(tempAct, false, 2, 0);
        display.setSegments(celsiusSymbol, 2, 2);  
      }      
    }            
  }  
  digitalWrite(rele_peltier, RELE_ON);
  digitalWrite(rele_fans, RELE_ON);
  
  while ((temp > temp_corte) || (temp < temp_min)){ //mientras no llegue a temperatura de corte o haya un error en la lectura del sensor
    temp = medirTemperaturaGabinete();   
    if  (temp > temp_min) {     
      tempAnt = tempAct;
      tempAct = temp;      
      if (tempAct != tempAnt){
        display.showNumberDec(tempAct, false, 2, 0);
        display.setSegments(celsiusSymbol, 2, 2);  
      }  
    }      
  }
  digitalWrite(rele_peltier, RELE_OFF);
  digitalWrite(rele_fans, RELE_OFF); 
}
float medirTemperaturaGabinete(){
  byte i;
  byte present = 0;
  byte type_s;
  byte data[9];
  byte addr[8];
  float celsius;
  
  if ( !dsGabinete.search(addr)) {    
    dsGabinete.reset_search();
    delay(250);
    return -127.0;
  }  
  if (OneWire::crc8(addr, 7) != addr[7]) {      
      return -127.0;
  } 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:      
      type_s = 1;
      break;
    case 0x28:      
      type_s = 0;
      break;
    case 0x22:      
      type_s = 0;
      break;
    default:      
      return -127.0;
  } 

  dsGabinete.reset();
  dsGabinete.select(addr);
  dsGabinete.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = dsGabinete.reset();
  dsGabinete.select(addr);    
  dsGabinete.write(0xBE);         // Read Scratchpad
  
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = dsGabinete.read();    
  }
 

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  return celsius;
}
/*float medirTemperaturaDisipador(){
  byte i;
  byte present = 0;
  byte type_s;
  byte data[9];
  byte addr[8];
  float celsius;
  
  if ( !dsDisipador.search(addr)) {    
    dsDisipador.reset_search();
    delay(250);
    return -127.0;
  }  
  if (OneWire::crc8(addr, 7) != addr[7]) {      
      return -127.0;
  } 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:      
      type_s = 1;
      break;
    case 0x28:      
      type_s = 0;
      break;
    case 0x22:      
      type_s = 0;
      break;
    default:      
      return -127.0;
  } 

  dsDisipador.reset();
  dsDisipador.select(addr);
  dsDisipador.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = dsGabinete.reset();
  dsDisipador.select(addr);    
  dsDisipador.write(0xBE);         // Read Scratchpad
  
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = dsDisipador.read();    
  }
 

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  return celsius;
}
*/