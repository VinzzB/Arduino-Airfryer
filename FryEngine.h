/*
 * Copyright (c) 2022, Vincent Bloemen (VinzzB)
 * All rights reserved.
 *   
 * This source code is licensed under the Apache 2.0 license found in the
 * LICENSE file in the root directory of this source tree. 
 */
 
#ifndef FryEngine_h
  #define FryEngine_h
  #include "Arduino.h"
  #include "Product.h"

  //start heater when below this offset temperature. 
  //Lower = more precision
  //Using zero could damage the relays. Use a value above 1
  #define TEMP_OFFSET_LOW 5

  #define PREHEAT_COMPLETE_STEP -1
  #define ENGINE_STOPPED_STEP -2
  typedef void (*callback)(int);

  class FryEngine {
    
    public:
      FryEngine(byte heaterPin, byte fanPin, byte temperaturePin, int _preHeatTimeout, callback stepCompletedCallBack);

      void       setProduct(Product* product);
      void       start();
      void       start(Product* product);
      void       stop();
      bool       isRunning();
      bool       timer();
      unsigned int getElapsedSeconds();
      unsigned int getRemainingSeconds();
      CookStep*  getStep(byte stepIdx);
      CookStep*  getCurrentStep();
      byte       getCurrentStepIdx();
      byte       getStepsCount();
      bool       getPreHeatReached();
      bool       getPreHeat();
      void       setPreHeat(bool value);
      byte       getTemperature();
      bool       isOnTemperature();
      byte       resetTemperature();
     
    private:
      void       adjustHeat(); //checks if heater needs to ben on or off...   (in 'loop' function)
      void       powerFan(bool power); // fan on / off
      void       powerHeater(bool power);
      void       updateTemperature();
      byte       _heaterPin;
      byte       _fanPin;
      byte       _temperaturePin;
      int        _preHeatTimeout;
      int        _refreshInterval;
      unsigned long _refreshedOn; //timer for temperature adjustement.
      unsigned long _runningSince; //holds the starttime. (engine running)
      bool       _isOnTemp;
      byte       _temperatures[10]; //precision. (average temperature over 5s)
      byte       _tempIdx;
      bool       _preHeat;
      bool       _preHeatReached;
      unsigned long _preHeatReachedTime;
      byte       _stepsCount;
      byte       _currentStep;
      callback   _stepCompletedCallBackPtr;
      CookStep   _steps[MAX_STEPS];
  };
#endif
