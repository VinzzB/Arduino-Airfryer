/*
 * Copyright (c) 2022, Vincent Bloemen (VinzzB)
 * All rights reserved.
 *   
 * This source code is licensed under the Apache 2.0 license found in the
 * LICENSE file in the root directory of this source tree. 
 */
 
#ifndef LCD1602_h
  #define LCD1602_h
  #include "Arduino.h"
  #include <Wire.h> 
  #include <LiquidCrystal_I2C.h>
  #include "Product.h"
  #define BELL_CHAR 0x00
  #define HEAT_CHAR 0x01
  //#define PRODUCTNAME_MAX_LEN 15
  
  //enum screens
  #define SCREEN_MENU 0
  #define SCREEN_MENU_SAVE 1
  #define SCREEN_PRODUCT 2
  #define SCREEN_RUNNING 3
  #define SCREEN_EDIT_NAME 4
  #define SCREEN_EDIT_STEP 5
  #define SCREEN_EDIT_BEEP 6
  #define SCREEN_EDIT_TIME 7
  #define SCREEN_EDIT_TEMP 8
  #define SCREEN_EDIT_PREHEAT 9

  #define DIALOG_RESULT_ABORT -1
  #define DIALOG_RESULT_NO 0
  #define DIALOG_RESULT_YES 1
  

  class LCD1602 {    
    
    public:
      LCD1602(LiquidCrystal_I2C& _lcd);
      void init();
      void lcdPowerMode(bool on);
      void printRunLine(unsigned int elapsedSeconds, byte temperature, byte heatingSign);
      void printStepLine(byte stepIdx, long secToGo, byte temp, bool beep);
      void printProductLine(char* product, byte deviceTemperature, byte heatingSign);
      void printSaveDialog(short option = 0);
      void printMenu(char item[PRODUCTNAME_MAX_LEN]);
      //void openMenu();
      void changeChar(char value, byte x, byte y);
      bool menuBlinkItem;
      byte textEditIdx;
      byte current; //SCREEN_* Enum.
      
    private:
      LiquidCrystal_I2C &lcd;      
      void printDeviceTemperature(byte temperature, byte temperatureSign);
      void printCelcius(byte temp, byte sign, bool isDeviceTemp);
      void printTimerTime(unsigned int elapsedSeconds);
      void printTimeOnLcd(int &minutes, int &seconds);
      void printTime(long allSeconds, bool inEditMode);
      void calcTime(long allSeconds, int &outM, int &outS);
      void printTemperature(byte temp);
      void printLine(const __FlashStringHelper* item, byte maxLen);
      void printLine(char* item, byte maxLen);
      void printBeep(bool beep);
      void printStep(byte stepIdx);
      void printDialogOption(bool checked, const __FlashStringHelper* text);
      void clearChars(byte len);
  };

#endif
