/*
 * Copyright (c) 2022, Vincent Bloemen (VinzzB)
 * All rights reserved.
 *   
 * This source code is licensed under the Apache 2.0 license found in the
 * LICENSE file in the root directory of this source tree. 
 */
 
#include "Arduino.h"
#include "Product.h"
#include "FryEngine.h"
#include <math.h>

FryEngine::FryEngine(byte heaterPin, byte fanPin, byte temperaturePin, int preHeatTimeout, callback stepCompletedCallBack) {
  _stepCompletedCallBackPtr = stepCompletedCallBack;
  _heaterPin = heaterPin;
  _fanPin = fanPin;
  _temperaturePin = temperaturePin;
  _refreshInterval = 500;
  _currentStep = 0;
  _preHeatTimeout = preHeatTimeout;
  pinMode(_heaterPin,OUTPUT);
  pinMode(_fanPin,OUTPUT);
  pinMode(_temperaturePin, INPUT);
  resetTemperature();
}

byte FryEngine::resetTemperature() {
  for(int x = 0; x <10; x++) { updateTemperature(); }
  return getTemperature();
}

void FryEngine::setProduct(Product* product) {

  _preHeat = product->preHeat;
  //copy all steps
  for(_stepsCount = 0; _stepsCount < product->stepsCount; _stepsCount++) {  //&& _stepsCount < MAX_STEPS
    memcpy(&_steps[_stepsCount], &product->steps[_stepsCount], 4);
  }
}

void FryEngine::start() {
  _currentStep = 0;
  _runningSince = millis();
  _preHeatReached = false;
  _preHeatReachedTime = 0;
  powerFan(getCurrentStep()->temp>0);
}

void FryEngine::start(Product* product) {
  setProduct(product);  
  start();	
}

void FryEngine::stop() { 
  _currentStep = 0;
  _runningSince = 0;
  powerFan(0);
  powerHeater(0);
  _stepCompletedCallBackPtr(ENGINE_STOPPED_STEP);
}

unsigned int FryEngine::getElapsedSeconds() {
  return _runningSince == 0 ? 0 : (millis() - _runningSince) / 1000 ;
}

unsigned int FryEngine::getRemainingSeconds() {
  int allSecondsTillCurrentStep = 0;
  if(!isRunning()) return 0;
  for (int x = 0; x < getCurrentStepIdx(); x++) {
      allSecondsTillCurrentStep += _steps[x].timeInSec;
  }
  int remainingTime = getCurrentStep()->timeInSec - (getElapsedSeconds()-allSecondsTillCurrentStep );
  return remainingTime < 0 ? 0 : remainingTime;
}

CookStep* FryEngine::getCurrentStep() {
  return &_steps[_currentStep];
}

CookStep* FryEngine::getStep(byte stepIdx) {
  return &_steps[stepIdx];
}

bool FryEngine::getPreHeatReached() {
  return _preHeatReached;
}

bool FryEngine::getPreHeat() {
  return _preHeat;
}

void FryEngine::setPreHeat(bool value){
  _preHeat = value;
}

byte FryEngine::getCurrentStepIdx() {
  return _currentStep;
}

byte FryEngine::getStepsCount() {
  return _stepsCount;
}

void FryEngine::updateTemperature() {
  int val = analogRead(_temperaturePin);
  double temp;
  temp = log((100000.0/30)*((1024.0/val-1))); //100000 = 100k thermistor.
  temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * temp * temp ))* temp );
  temp -= 273.15;
  _temperatures[_tempIdx++ % 10] = constrain(temp, 0, 255);
}

byte FryEngine::getTemperature() {
  int totTemp = 0;
  for(int x = 0; x <10; x++) {
    totTemp += (byte)_temperatures[x];
  }
  return totTemp / 10;
}

bool FryEngine::isRunning() {
  return _runningSince > 0;
}

//return true to process timer event in ino script.
bool FryEngine::timer() {
  unsigned long refreshMillis = millis();
  if (refreshMillis - _refreshedOn >= _refreshInterval) {
    //do refresh actions!
    _refreshedOn = refreshMillis;
    
    //update current device temperature
    updateTemperature();
    
    //is engine running?
    if (isRunning()) {
      
      //is pre heating?
      if(getPreHeat()) {
          //reset running time untill Pre Heat mode is deactivated by user.
          _runningSince = millis();          
          unsigned int secPassed = (refreshMillis - _preHeatReachedTime) / 1000;
          //stop the engine when [PreHeatTimeout] seconds passed since temperature was reached without any user interaction.
          if(_preHeatReached && secPassed >= _preHeatTimeout){
              stop();
              return true;
          }
          //pre heat complete?
          if(!_preHeatReached && getTemperature() >= getCurrentStep()->temp){
            _preHeatReached = true;
            _preHeatReachedTime = refreshMillis;
            _stepCompletedCallBackPtr(PREHEAT_COMPLETE_STEP);
          }
      }
      
      //Is Step complete?
      if (getRemainingSeconds() <= 0){        
        int completedStep = _currentStep;
        //Get the next step that contains time and store the index in _currentStep.
        while(++_currentStep < getStepsCount() && getCurrentStep()->timeInSec==0) { }
        //custom callback to ino script (eg for buzzer)
        _stepCompletedCallBackPtr(completedStep);
        //Are we trhrough all the steps?
        if(_currentStep >= getStepsCount()) {
          stop();
        } else {
          //reset isOnTemp when starting a new step.
          _isOnTemp = getTemperature() >= getCurrentStep()->temp;
          //set fan OFF when temperature esuals zero.
          powerFan(getCurrentStep()->temp>0);
        }
      }
      //check if fryer is still on temperature.
      adjustHeat();
    }  
    return true;
  }
  return false;
}

bool FryEngine::isOnTemperature() {
  return _isOnTemp;
}

void FryEngine::adjustHeat() {
  if(isRunning()){
    byte currentTemp = getTemperature();
    byte prefferedTemp = getCurrentStep()->temp;
    //is temperature greater than the preffered temperature?
    //or was the fryer on temperature and is the current temperature still above the preffered Temperature minus the offset (-5) 
    _isOnTemp = (currentTemp >= prefferedTemp) || (_isOnTemp && currentTemp > prefferedTemp-TEMP_OFFSET_LOW);
    powerHeater( !isOnTemperature() ); 
  }
}

void FryEngine::powerFan(bool power) {
  digitalWrite(_fanPin, power);
}

void FryEngine::powerHeater(bool power) {
  digitalWrite(_heaterPin, power);
}