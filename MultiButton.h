#ifndef MultiButton_h
  #define MultiButton_h
  #include "Arduino.h"
  //#include "IInputDevice.h"
  
  #define BTN_SINGLE_CLICK 1
  #define BTN_DOUBLE_CLICK 2
  #define BTN_LONG_PRESS 3
  
  class MultiButton {
    
    public:
      byte check();
      void setup(byte buttonPin);
      
    private:
      byte gpioPin;     
      // Button timing variables
      int debounce = 50;          // ms debounce period to prevent flickering when pressing or releasing the button
      int DCgap = 300;            // max ms between clicks for a double click event
      int holdTime = 750;        // ms hold period: how long to wait for press+hold event
      int longHoldTime = 20000;    // ms long hold period: how long to wait for press+hold even    
      // Button variables
      boolean buttonVal = HIGH;   // value read from button
      boolean buttonLast = HIGH;  // buffered value of the button's previous state
      boolean DCwaiting = false;  // whether we're waiting for a double click (down)
      boolean DConUp = false;     // whether to register a double click on next release, or whether to wait and click
      boolean singleOK = true;    // whether it's OK to do a single click
      long downTime = -1;         // time the button was pressed down
      long upTime = -1;           // time the button was released
      boolean ignoreUp = false;   // whether to ignore the button release because the click+hold was triggered
      boolean waitForUp = false;        // when held, whether to wait for the up event
      boolean holdEventPast = false;    // whether or not the hold event happened already
      boolean longHoldEventPast = false;// whether or not the long hold event happened already
  };
#endif