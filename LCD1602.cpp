/*
 * Copyright (c) 2022, Vincent Bloemen (VinzzB)
 * All rights reserved.
 *   
 * This source code is licensed under the Apache 2.0 license found in the
 * LICENSE file in the root directory of this source tree. 
 */
 
#include "Arduino.h"
#include "LCD1602.h"
#include <LiquidCrystal_I2C.h>

//Custom LCD Chars
byte bellOnChar[8] = {
  0,  
  B00100, 
  B01110, 
  B01110, 
  B11111, 
  B00100, 
  B00010, 
  0
};

byte preHeatSign[8] = {
  B00100,
  B01010,
  B01010,
  B01010,
  B10001,
  B10101,
  B10001,
  B01110
};

LCD1602::LCD1602(LiquidCrystal_I2C& _lcd) : lcd(_lcd) { }

void LCD1602::init() {
  lcd.begin(16,2);
  lcd.createChar(BELL_CHAR,bellOnChar);
  lcd.createChar(HEAT_CHAR,preHeatSign);
  lcd.backlight();
  lcdPowerMode(true);
  lcd.clear();
  //Print splash screen
  lcd.print(F(" A r d u i n o "));
  delay(250);
  lcd.setCursor(0,1); //x,y
  lcd.print(F("A I R F R Y E R!"));
  delay(1500);
  lcd.clear();
}

void LCD1602::lcdPowerMode(bool on) {
  if(on) lcd.on();
  else lcd.off();   
  lcd.setBacklight(on ? 255 :0);
}

void LCD1602::printDeviceTemperature(byte temperature, byte temperatureSign) {
  lcd.setCursor(10,0);
  printCelcius(temperature, temperatureSign, true);
}

void LCD1602::printCelcius(byte temp, byte sign, bool isDeviceTemp) {
  bool isEditMode = current == SCREEN_EDIT_PREHEAT && isDeviceTemp;
  char usedSign = menuBlinkItem && isEditMode ? '_' : sign;
  char buffer[7]; // !000°C = 6 chars + null terminator;
  sprintf(buffer, "%c%3d%cC", usedSign, temp, (char)223);
  lcd.print(buffer);
}

//Prints the time at the current position on the screen. consumes 5 digits on display!
void LCD1602::printTimeOnLcd(int &minutes, int &seconds) {
  char time[7];
  sprintf(time, "%02d:%02d" , minutes, seconds);
  lcd.print(time);  
  if(minutes < 100) lcd.print(' ');
}

void LCD1602::printTimerTime(unsigned int elapsedSeconds) {  
  lcd.setCursor(4,0); 
  printTime(elapsedSeconds, 0);
}

void  LCD1602::printTime(long allSeconds, bool inEditMode) {
  //calc time and print on screen
  int seconds, minutes; //TODO: no hours yet... 
  calcTime(allSeconds,minutes,seconds);
  if(inEditMode && menuBlinkItem)
    clearChars(6);
  else
    printTimeOnLcd(minutes,seconds);       
}

//calc time, returns minutes and seconds as references.
void LCD1602::calcTime(long allSeconds, int &outM, int &outS) {
  outS = allSeconds; //store seconds
  outM = (outS / 60); //calc minutes (decimal numbers are converted to round number (lower bound))
  outS = outS - (outM * 60); //remove the minutes from s;
}

void  LCD1602::printTemperature(byte temp) {
  if(current == SCREEN_EDIT_TEMP && menuBlinkItem)
    printLine(F("@"), 6);
  else
    printCelcius(temp,'@', false); 
}


void LCD1602::printBeep(bool beep){
  lcd.write(current == SCREEN_EDIT_BEEP && menuBlinkItem ? '_' : beep ? (byte)BELL_CHAR : ' ');
}

void LCD1602::printStep(byte stepIdx) {
  if(current == SCREEN_EDIT_STEP == 1 && menuBlinkItem) {
    clearChars(3);
  } else {
    char stepChars[4];
    sprintf(stepChars, "S%02d", stepIdx+1);
    lcd.print(stepChars);
  }  
}

/*
  prints one step as one liner on the display. Format:
  ------------------
  |P00 99:59 @200°C|
  ------------------
*/
void LCD1602::printStepLine(byte stepIdx, long secToGo, byte temp, bool beep) {
  lcd.setCursor(0,1);
  printStep(stepIdx);
  lcd.setCursor(3,1);
  printBeep(beep);
  lcd.setCursor(4,1); 
  printTime(secToGo,current == SCREEN_EDIT_TIME);
  lcd.setCursor(10,1);
  printTemperature(temp);
}

void LCD1602::printProductLine(char* product, byte deviceTemperature, byte heatingSign){
  lcd.setCursor(0,0);
  char text[11] = {};
  memcpy(text, product, 10);
  printLine(text,10);
  printDeviceTemperature(deviceTemperature, heatingSign);   
}

void LCD1602::printRunLine(unsigned int elapsedSeconds, byte temperature, byte heatingSign){  
  //erase productname.
  lcd.setCursor(0,0);
  clearChars(4);
  //print timer
  printTimerTime(elapsedSeconds);
  //print device temp
  printDeviceTemperature(temperature, heatingSign);   
}

void LCD1602::printMenu(char item[PRODUCTNAME_MAX_LEN]) {
  lcd.setCursor(0,0);
  printLine(F("Choose product:"),16);  
  lcd.setCursor(0,1);
  printLine(F(">|"),2);
  printLine(item,14);
}

void LCD1602::changeChar(char value, byte x, byte y) {
  lcd.setCursor(x,y);
  lcd.print(value);
  lcd.setCursor(x,y);
}

void LCD1602::clearChars(byte len) {
  for(byte x = 0; x < len; x++) {      
    lcd.print(' ');
  }  
}

void LCD1602::printLine(const __FlashStringHelper* item, byte maxLen) {
  byte len = lcd.print(item);
  clearChars(maxLen - len);
}

void LCD1602::printLine(char* item, byte maxLen) {
  byte len = lcd.print(item);
  clearChars(maxLen - len); 
}

void LCD1602::printSaveDialog(short option = 0) {
  lcd.setCursor(0,0);
  printLine(F("Save changes?"),16);
  lcd.setCursor(0,1);
  printDialogOption(option == DIALOG_RESULT_YES, F("YES"));
  printDialogOption(option == DIALOG_RESULT_NO, F("NO"));
  printDialogOption(option == DIALOG_RESULT_ABORT, F("ABORT"));  
}

void LCD1602::printDialogOption(bool checked, const __FlashStringHelper* text) {
  lcd.print(checked ? '<' : ' ');
  lcd.print(text);
  lcd.print(checked ? '>' : ' ');
}
