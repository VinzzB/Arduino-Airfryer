#include "Arduino.h"
#include "Eeprom_cookbook.h" 

static const byte EEPROM_Cookbook::eeprom_check[] = { 'A','A','I','R', 1, MAX_STEPS }; 

EEPROM_Cookbook::EEPROM_Cookbook(int max_eeprom_bytes) {	
  _max_eeprom_bytes = max_eeprom_bytes;  
}

int EEPROM_Cookbook::getProductAddress(byte productIdx) {
    return sizeof(eeprom_check) + (productIdx * productSize);
}

void EEPROM_Cookbook::readProduct(byte productIdx, Product* p) {
  
    int pos =  getProductAddress(productIdx);

    //read name
    char name[PRODUCTNAME_MAX_LEN] = {};    
    for(byte x = 0; x < PRODUCTNAME_MAX_LEN; x++) 
      name[x] =EEPROM.read(pos + x);
    
    memcpy(p->name, name, strlen(name)+1);  
    //p->setName(name);
    pos += PRODUCTNAME_MAX_LEN - 1; // 14;
    
    p->preHeat = EEPROM.read(pos++);
    p->stepsCount = EEPROM.read(pos++);
    
    for(int x = 0; x < p->stepsCount; x++) {
      CookStep tempStep = {};      
      EEPROM.get(pos, tempStep);
      memcpy(&p->steps[x], &tempStep, sizeof(tempStep));

      pos += sizeof(CookStep); // 4;
    }

}

void EEPROM_Cookbook::writeProduct(byte productIdx, Product p) {
  
  //get address for first product
  int pos = getProductAddress(productIdx);
  
  //write name
  writeCharArray(pos,p.name,strlen(p.name)+1); // string + null char    
  pos += PRODUCTNAME_MAX_LEN - 1; // 14;
  
  //byte stepCount = 8; // = MAX_STEPS!
  EEPROM.update(pos++, p.preHeat);
  EEPROM.update(pos++, p.stepsCount);
  
  //write steps
  for(uint8_t x = 0; x < p.stepsCount; x++) {
    EEPROM.put(pos, p.steps[x]);
    pos += sizeof(CookStep); // 4;
  }

}

void EEPROM_Cookbook::writeCharArray(int address, char* text, int len) {
  for(uint8_t x = 0; x < len; x++)
    EEPROM.update(address + x, text[x]);
}

int EEPROM_Cookbook::count() {
    int maxBytes = _max_eeprom_bytes - sizeof(eeprom_check);
    return maxBytes / productSize;
}


bool EEPROM_Cookbook::containsData() {    
  for(uint8_t x = 0; x < sizeof(eeprom_check); x++) {
    if(EEPROM[x] != eeprom_check[x])
      return false;
  }
  return true;
}

void EEPROM_Cookbook::prepareEEPROM(bool force = false) {
  if(!containsData() || force) {    
    int maxEntries = count();    
    for(byte x = 0; x < maxEntries; x++) {
      int pos = getProductAddress(x);

      char name[PRODUCTNAME_MAX_LEN] = "Custom ";       
      itoa(x+1, name+7, 10);

      writeCharArray(pos,name,strlen(name)+1); // string + null char    
      pos += PRODUCTNAME_MAX_LEN - 1; // 14;
      
      EEPROM.update(pos++, 0);
      EEPROM.update(pos++, MAX_STEPS);
  
      for(int i = 0; i < MAX_STEPS * sizeof(CookStep); i++ )
        EEPROM.update(pos+i, 0);

      //Write eeprom check as last. (when all structs are written)
      writeCharArray(0,eeprom_check, sizeof(eeprom_check));
    }    
  }
}