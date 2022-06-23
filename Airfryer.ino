
/*
 * 
 * Philips Airfryer. Model: HD9240
 * Creator: Vincent Bloemen (VinzzB / vinzz.be)
 * 
 * Airfryer components :
 * 1x 230Vc motor
 * 1x 230Vc Spiral heating coil
 * 1x Temperature sensor
 * 
 * 
 * Circuit:
 * - A1: Temperature Sensor
 * - A4: LiquidCrystal Display - Data (SDA)
 * - A5: LiquidCrystal Display - Clock (SCL)
 * - D2: Rotary Encoder - Switch (SW)
 * - D3: Rotary Encoder - Data (DT)
 * - D4: Rotary Encoder - Clock (CLK)
 * - D7: Fan Relay  / Blue led
 * - D8: Heater Relay / Red led
 * - D9: PWM Piezo Speaker (SPK) 
 * 
 * LEDs:
 * - D8 -> 220 Ohm -> Red Led -> Ground
 * - D7 -> 220 Ohm -> Blue Led -> Ground
 * 
 * REQUIRED LIBRARIES
 * - LiquidCrystal I2C (optionally from https://github.com/fmalpartida/New-LiquidCrystal if needed)
 * - Encoder
 *
 */

/* INCLUDES */
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include "FryEngine.h"
#include "LCD1602.h"
#include "MultiButton.h"
#include <Encoder.h>
#include "Eeprom_cookbook.h"

/* PROPERTIES */
const byte heaterPin      = 8;     //D8 = pin 14
const byte fanPin         = 7;     //D7 = pin 13
const byte buttonPin      = 2;     //D2 = pin 04 - Interrupt 0 (!)
const byte tempSensorPin  = A1;    //A1 = pin 24
const byte speakerPin     = 9;     //D9 = pin 15
const byte rotaryDatPin   = 3;     //D3 = pin 05 - Interrupt 1 (!)
const byte rotaryClkPin   = 4;     //D4 = pin 06

const bool invertRotary   = false; //swap rotary direction.
const int buzzFrequency   = 3520;  //buzzer frequency (3520 = NOTE_A7)
const int exitEditDelay   = 10;    //in seconds! 
const int powerOffTimeout = 60;    //in seconds!
const int preHeatTimeout  = 300;   //in seconds!
const int tempSteps[]     = {1,  5, 10,  20,  30,  40,  50,  60}; //rotary intervals. (slow > fast rotations)
const int timeSteps[]     = {1, 15, 60, 120, 180, 240, 300, 360}; //rotary intervals. (slow > fast rotations)

LiquidCrystal_I2C lcd(0x27, 16, 2); //find I2C address with I2C_scanner script
//LiquidCrystal_I2C lcd(0x20, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); 

/* FOODLIST */
//max steps per product. See MAX_STEPS in Product.h
Product           product = { "Custom 0", 0, MAX_STEPS };
EEPROM_Cookbook   cookbook(1024);

/* GLOBAL VARS */
LCD1602           screen(lcd);
FryEngine         engine(heaterPin, fanPin, tempSensorPin, preHeatTimeout, &stepCompletedCallBack);
Encoder           rotary(rotaryDatPin, rotaryClkPin);
MultiButton       button;           //rotary button.
byte              menuProductIdx    = 0;
byte              menuStepIdx       = 0;
byte              ADCSRA_State;     //used for hibernate state.
long              oldPosition       = -999; //rotary last position
byte              switchPreHeatText = 0; //counter for switching preheat text on screen
unsigned long     editingSince      = 0;
unsigned long     lastActionOn      = millis();
bool              isDirty           = false;
short             dialogResult      = 0;

void setup() {  
    
  //SERIAL 
  delay(100);
  Serial.begin(2000000); 

  //SPEAKER
  pinMode(speakerPin,OUTPUT); //piezo
  tone(speakerPin, buzzFrequency, 500);

  //ROTARY ENCODER AND BUTTON
  pinMode(rotaryDatPin, INPUT_PULLUP);
  pinMode(rotaryClkPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonPin), wakeUpInterrupt, FALLING); //wake-up interrupt.
  button.setup(buttonPin);
  
  //TEMPERATURE SENSOR
  engine.resetTemperature();

  //LCD SCREEN
  screen.init();

  //EEPROM  
  if(!cookbook.containsData()) {
    lcd.setCursor(0,0);
    lcd.print(F("Preparing EEPROM"));
    lcd.setCursor(0,1);
    lcd.print(F("structure..."));
    cookbook.prepareEEPROM();
  }
  cookbook.readProduct(menuProductIdx, &product);
  screen.printMenu(product.name); //show menu on startup
}

void loop() {

  //go to sleep?
  checkSleepMode();  
  
  //exit edit mode when idle for [exitEditDelay] seconds.
  bool exitEdit = (millis() - editingSince) / 1000 >= exitEditDelay;
  if(screen.current > SCREEN_RUNNING && exitEdit) {
    if(screen.current == SCREEN_EDIT_NAME){
      screen.current = SCREEN_MENU;
    } else if(engine.isRunning()) {
      menuStepIdx = engine.getCurrentStepIdx(); //go back to the step being processed by the engine.
      screen.current = SCREEN_RUNNING;     
    } else {
      screen.current = SCREEN_PRODUCT;
    }    
    lcd.noBlink();
  }

  //Process engine ticks and update screen.
  if(engine.timer()) {
    //Update running screen
    screen.menuBlinkItem = !screen.menuBlinkItem;
    if(screen.current != SCREEN_MENU 
    && screen.current != SCREEN_MENU_SAVE
    && screen.current != SCREEN_EDIT_NAME)
      printRunDisplay();
  }
  
  //reset hibernate timeout when engine is running
  if(engine.isRunning())
    lastActionOn = millis();
  
  //execute user interactions
  userInteraction();
}

void checkSleepMode() {
  //Did device State changed
  bool powerOff = (millis() - lastActionOn) / 1000 >= powerOffTimeout;
  if(powerOff) {
      //Serial.println("Hibernate");      
      screen.lcdPowerMode(false);
      goToSleep(); 
      //wakes-up here...        
      lastActionOn = millis();  // set last action time. 
      engine.resetTemperature(); //reset temp buffer.
      screen.lcdPowerMode(true);         
      rotary.write(0); //reset rotary movements
      //Serial.println("Woke up from hibernate");        
      //wait for button release (= LOW while pressed)
      while(!digitalRead(buttonPin)) { delay(10); };            
  }
}

void goToSleep() {
  //https://www.youtube.com/watch?v=urLSDi7SD8M
  //init sleep mode
  __asm__("sei");                         //make sure interrupts are on  (awaker)
  ADCSRA_State = ADCSRA;                  //store current register config
  ADCSRA &= ~(1<<7);                      // disable ADC // ~ = inverted
  SMCR |= (1<<2);                         //power down mode ('1' shift 2 places as OR (only changes second bit))
  SMCR |= 1;                              //enable sleep; (first bit)
  MCUCR |= (3 << 5);                      //set BODS and BODSE (shift 11 with 5 places)
  MCUCR = (MCUCR & ~(1 << 5)) | (1 << 6); //set BODS bit and clear BODSE bit.   
  __asm__("sleep");                       //Goodnight! (inline assembler: executes sleep command)  
  //SLEEPING....
  //awakes here when interrupted!
  SMCR |= 0;                              //disable sleep; (first bit)  
  ADCSRA = ADCSRA_State;                  //restore ADC.   
}

void wakeUpInterrupt() {/* Just an empty wake-up interrupt! */}

//only allow additions and substractions above zero.
byte safeAdd(byte value, int addValue, byte maxValue) { 
    byte newValue = value + addValue;
    return value == 0 && addValue < 0 ? 0 : newValue > maxValue ? maxValue : newValue;
}

short readRotaryPosition() {
  
  // Read rotary state
  long newPosition = round(rotary.read()/(double)8);
  short rotaryPosition = 0;
  
  //change direction (if configured) and store postion
  if(newPosition != oldPosition)
    oldPosition = rotaryPosition = invertRotary ? -newPosition : newPosition;        
    
  // Reset rotary state
  if(rotaryPosition != 0)
    rotary.write(0);
    
  return rotaryPosition;
}

void userInteraction() {

  // Read button & rotary state    
  int buttonState = button.check();
  short rotaryPosition = readRotaryPosition();    
  int direction = rotaryPosition < 0 ? -1 : 1; //Using an INT type for time calculations! 
  
  switch (screen.current) {

    case SCREEN_MENU: 
    /* =================================================================
     * Rotate rotary = scroll through products.
     * Pressed once  = Load selected item without starting engine
     * Pressed twice = Load step and start engine. 
     * Long press = edit product name.
     * =================================================================  */
      
      //Rotary actions in menu-
      if(rotaryPosition != 0) {
        if(isDirty) {
          screen.current = SCREEN_MENU_SAVE;
          screen.printSaveDialog(dialogResult = 0);          
        } else {
          menuProductIdx = safeAdd(menuProductIdx, direction, cookbook.count()-1);
          cookbook.readProduct(menuProductIdx, &product);
          screen.printMenu(product.name);        
        }
      }
      
      //button actions in menu.
      switch (buttonState)  {  
        
        case BTN_SINGLE_CLICK: /* SELECT */
        case BTN_DOUBLE_CLICK: /* SELECT & RUN */ {
          bool startEngine = buttonState == BTN_DOUBLE_CLICK;
          screen.current = startEngine ? SCREEN_RUNNING : SCREEN_PRODUCT;
          if(startEngine)
            engine.start(&product);             
          menuStepIdx = 0; //reset to first step
          lcd.clear();
          printRunDisplay();
          break;
        }
        case BTN_LONG_PRESS: /* ENTER NAME EDITOR */ 
          lcd.setCursor(2,1);
          lcd.blink();
          screen.textEditIdx = 0;
          screen.current = SCREEN_EDIT_NAME;
          break;          
      }
        
      break;
    
    case SCREEN_MENU_SAVE: {
    /* =================================================================
     * Rotate rotary = change dialog option
     * Pressed once  = confirm option     
     * ================================================================= */

      if(rotaryPosition != 0) {
        short prevDialogResult = dialogResult;
        dialogResult = constrain(dialogResult-direction, -1, 1);
        if(prevDialogResult != dialogResult)
          screen.printSaveDialog(dialogResult);
      }
      
      if(buttonState == BTN_SINGLE_CLICK) {
        switch(dialogResult) {
          case DIALOG_RESULT_NO:  cookbook.readProduct(menuProductIdx, &product); break;
          case DIALOG_RESULT_YES: cookbook.writeProduct(menuProductIdx, product); break;
        }
        isDirty = dialogResult == DIALOG_RESULT_ABORT;
        screen.current = SCREEN_MENU; //todo: should go into LCD1602 class. PrintMenu() = Screen Menu...
        screen.printMenu(product.name);
      }

      break;
    }
    case SCREEN_PRODUCT:
    /* =================================================================
     * Rotate rotary = Enter edit mode and change time
     * Pressed once  = Start engine
     * Pressed twice = Open menu 
     * Long press    = Enter edit mode
     * ================================================================= */

      if(rotaryPosition != 0)
        screen.current = SCREEN_EDIT_TIME;
        
      switch (buttonState) {
        
        case BTN_SINGLE_CLICK: /* START ENGINE */
          screen.current = SCREEN_RUNNING;
          menuStepIdx = 0;          
          engine.start(&product);                   
          break;
        
        case BTN_DOUBLE_CLICK: /* ENTER MENU */
          screen.current = SCREEN_MENU; 
          screen.printMenu(product.name);
          break;
          
        case BTN_LONG_PRESS: /* ENTER EDIT MODE */
          screen.current = SCREEN_EDIT_STEP;
          break;
      }
      break;
    
    case SCREEN_RUNNING:
    /* =================================================================
     * Rotate rotary = Enter edit mode and change time
     * Pressed once  = Skip preheat stage / Stop engine       
     * Pressed twice = Enter edit mode         
     * Long press    = Enter edit mode
     * ================================================================= */
      
      if(rotaryPosition != 0)
        screen.current = SCREEN_EDIT_TIME;
        
      switch(buttonState) {
        
        case BTN_SINGLE_CLICK: /* EXIT PREHEAT STAGE OR STOP ENGINE */
          if(engine.getPreHeat()){
            engine.setPreHeat(false);
          } else {
            engine.stop();
          }
          break;
          
        case BTN_DOUBLE_CLICK:
        case BTN_LONG_PRESS: /* ENTER EDIT MODE */
          screen.current = SCREEN_EDIT_STEP;
          break;
      }        
      break;

    case SCREEN_EDIT_NAME:
    /* =================================================================
     * Rotate rotary = roll current char
     * Pressed once  = select next char
     * Pressed twice = select next char (+2)
     * Long press    = Exit edit mode
     * Timeout timer = Exit edit mode. (see exitEdit in loop function)
     * ================================================================= */
      
      //change char at current string position
      if(rotaryPosition != 0) {
        char currChar = rollNameChar(screen.textEditIdx, rotaryPosition);
        screen.changeChar(currChar, screen.textEditIdx + 2 ,1);
        isDirty = true;
      }

      switch (buttonState) {
        
        case BTN_SINGLE_CLICK:              
        case BTN_DOUBLE_CLICK: /* CHANGE CHAR POSITION */
          screen.textEditIdx += buttonState;
          if(screen.textEditIdx > PRODUCTNAME_MAX_LEN - 2)
            screen.textEditIdx = 0;
          lcd.setCursor(screen.textEditIdx + 2, 1);
          break;
          
        case BTN_LONG_PRESS:  /* EXIT NAME EDITOR */
          screen.current = SCREEN_MENU;
          lcd.noBlink();
          break;
      }
      break;
         
    case SCREEN_EDIT_STEP ... SCREEN_EDIT_PREHEAT: {
    /* =================================================================
     * Rotate rotary = Change value for current edit field
     * Pressed once  = Select next edit field. (Step > Melody > Time > Temperature > PreHeat)
     * Pressed twice = select edit field +2
     * Long press    = Exit edit mode
     * Timeout timer = Exit edit mode. (see exitEdit in loop function)
     * ================================================================= */

      if(rotaryPosition != 0) {      
        
        //Product* product = &products[menuProductIdx];
        CookStep* currStep = engine.isRunning() ? engine.getStep(menuStepIdx) : &product.steps[menuStepIdx];
        
        switch(screen.current) {
          case SCREEN_EDIT_STEP: menuStepIdx = safeAdd(menuStepIdx, direction, product.stepsCount - 1); break;
          case SCREEN_EDIT_BEEP: currStep->beep = !currStep->beep ; break;
          case SCREEN_EDIT_TIME: { //brackets needed for scoped var!
            int newTime = currStep->timeInSec + (direction * timeSteps[abs(rotaryPosition)-1]);
            currStep->timeInSec = newTime < 0 ? 0 : newTime;
            break;
          }
          case SCREEN_EDIT_TEMP: currStep->temp = currStep->temp + (direction * tempSteps[abs(rotaryPosition)-1]); break;
          case SCREEN_EDIT_PREHEAT: product.preHeat = !product.preHeat; break;
        }
        
        isDirty |= !engine.isRunning() && screen.current > SCREEN_EDIT_STEP;        
        screen.menuBlinkItem = false; //delay blink when editing (true = hidden)    
        printRunDisplay();
      } 

      switch(buttonState){
        
        case BTN_SINGLE_CLICK: /* CHANGE EDIT FIELD */
        case BTN_DOUBLE_CLICK: {
          byte newScreenIdx = screen.current + buttonState;
          if(newScreenIdx > SCREEN_EDIT_PREHEAT || ( engine.isRunning() && newScreenIdx == SCREEN_EDIT_PREHEAT ))
            newScreenIdx = SCREEN_EDIT_STEP;
          screen.current = newScreenIdx;
          break;
        }
        
        case BTN_LONG_PRESS: { /* EXIT EDIT MODE */
          screen.current = engine.isRunning() ? SCREEN_RUNNING : SCREEN_PRODUCT;
          if(engine.isRunning())
            menuStepIdx = engine.getCurrentStepIdx();
          break;
        }
        break;        
      }
    }
  }

  //reset timers
  if(buttonState > 0 || rotaryPosition != 0){
    // Reset powerOff timer
    lastActionOn = millis();      
    // Reset edit timer.
    if(screen.current > SCREEN_RUNNING) {
      editingSince = millis();
      screen.menuBlinkItem = false;
    }
  }
  
} /* END userInteraction() */

char rollNameChar(byte pos, short roll) {      
  char c = product.name[pos] + roll;
  if(c < 32) c = 126;
  if(c > 126) c = 32;
  return product.name[pos] = c;
}

void printRunDisplay() {
  //First line
  if(engine.isRunning()) {
    byte tempSign = engine.isOnTemperature() ? '<' : '>';
    if(engine.getPreHeat()) {  
      char* actionText = engine.getPreHeatReached() ? "START NOW?" : "!PREHEAT!";
      screen.printProductLine(++switchPreHeatText % 10 > 4 ? product.name : actionText, engine.getTemperature(), tempSign);
    } else {
      screen.printRunLine(engine.getElapsedSeconds(), engine.getTemperature(), tempSign);
    }
  } else {
      screen.printProductLine(product.name, engine.getTemperature(), product.preHeat ? HEAT_CHAR : ' ');
  }
  
  //second line
  CookStep* currStep = engine.isRunning() ? engine.getStep(menuStepIdx) : &product.steps[menuStepIdx];   
  screen.printStepLine(menuStepIdx,
     engine.isRunning() && menuStepIdx == engine.getCurrentStepIdx() ? engine.getRemainingSeconds() : currStep->timeInSec, 
     currStep->temp,
     currStep->beep); 
}

void stepCompletedCallBack(int stepIdx) {

  //Actions when engine stops.
  if(stepIdx == ENGINE_STOPPED_STEP) {
    screen.current = SCREEN_PRODUCT;
    menuStepIdx = 0;
  } 
  
  //show next step on screen
  if (stepIdx == menuStepIdx){
    menuStepIdx = engine.getCurrentStepIdx() ;
  }
  
  //Buzz? (preHeat & steps with buzz option)
  if (stepIdx == PREHEAT_COMPLETE_STEP 
  || (stepIdx >= 0 && engine.getStep(stepIdx)->beep)) {
    tone(speakerPin, buzzFrequency, 2000);
  }
}
