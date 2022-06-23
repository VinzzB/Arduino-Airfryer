/*
 * Copyright (c) 2022, Vincent Bloemen (VinzzB)
 * All rights reserved.
 *   
 * This source code is licensed under the Apache 2.0 license found in the
 * LICENSE file in the root directory of this source tree. 
 */
 
#ifndef eeprom_cookbook_h
  #define eeprom_cookbook_h
  #include "Arduino.h"
  #include <EEPROM.h>
  #include "Product.h"

    /*
    * EEPROM STRUCTURE:
   * Byte 0-5: AAIR01 
   * Byte6-*: Product struct[]
   * PRODUCT STRUCTURE:
   * - Name       14 bytes
   * - PreHeat    1 byte
   * - StepCount  1 byte
   * - steps      4 bytes * StepCount  (8 steps = 32 bytes)
   * TOTAL with 8 steps: 48 bytes = 21 products or 1008 bytes.  (Atmega328P has 1024 bytes)
   * TOTAL with 5 steps: 36 bytes = 28 products or 1008 bytes.  (Atmega328P has 1024 bytes)
   * 
   */
  
   class EEPROM_Cookbook {
    
    public:
  		EEPROM_Cookbook(int max_eeprom_bytes);
      void prepareEEPROM(bool force = false);
      void writeProduct(byte productIdx, Product p);
      void readProduct(byte productIdx, Product* p);      
      bool containsData();
      int getProductAddress(byte productIdx);
      int count();
		
    private:            
      int _max_eeprom_bytes;

      //14 bytes for name (without null terminator) + 1 byte preHeat + 1 byte stepsCount + (4 bytes * steps)
      static const int productSize = (PRODUCTNAME_MAX_LEN - 1) + 2 + (sizeof(CookStep) * MAX_STEPS); 
      void writeCharArray(int address, char* text, int len);

      static const byte eeprom_check[];
  };

#endif
