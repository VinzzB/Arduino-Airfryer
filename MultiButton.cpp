//=================================================
//  MULTI-CLICK:  One Button, Multiple Events
// from: http://forum.arduino.cc/index.php?topic=14479.0
// original: http://jmsarduino.blogspot.com/2009/10/4-way-button-click-double-click-hold.html

#include "Arduino.h"
#include "MultiButton.h"

void MultiButton::setup(byte buttonPin) {
  this->gpioPin = buttonPin;
  pinMode(buttonPin, INPUT_PULLUP);
}

byte MultiButton::check() {    
   byte event = 0;
   buttonVal = digitalRead(gpioPin);
   // Button pressed down
   if (buttonVal == LOW && buttonLast == HIGH && (millis() - upTime) > debounce)
   {
       downTime = millis();
       ignoreUp = false;
       waitForUp = false;
       singleOK = true;
       holdEventPast = false;
       longHoldEventPast = false;
       if ((millis()-upTime) < DCgap && DConUp == false && DCwaiting == true)  DConUp = true;
       else  DConUp = false;
       DCwaiting = false;
   }
   // Button released
   else if (buttonVal == HIGH && buttonLast == LOW && (millis() - downTime) > debounce)
   {        
       if (not ignoreUp)
       {
           upTime = millis();
           if (DConUp == false) DCwaiting = true;
           else
           {
               event = BTN_DOUBLE_CLICK;
               DConUp = false;
               DCwaiting = false;
               singleOK = false;
           }
       }
   }
   // Test for normal click event: DCgap expired
   if ( buttonVal == HIGH && (millis()-upTime) >= DCgap && DCwaiting == true && DConUp == false && singleOK == true && event != 2)
   {
       event = BTN_SINGLE_CLICK;
       DCwaiting = false;
   }
   // Test for hold
   if (buttonVal == LOW && (millis() - downTime) >= holdTime) {
       // Trigger "normal" hold
       if (not holdEventPast)
       {
           event = BTN_LONG_PRESS;
           waitForUp = true;
           ignoreUp = true;
           DConUp = false;
           DCwaiting = false;
           //downTime = millis();
           holdEventPast = true;
       }
       /* Trigger "long" hold
       if ((millis() - downTime) >= longHoldTime)
       {
           if (not longHoldEventPast)
           {
               event = 4;
               longHoldEventPast = true;
           }
       }*/
   }
   buttonLast = buttonVal;
   return event;
}