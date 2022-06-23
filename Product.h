/*
 * Copyright (c) 2022, Vincent Bloemen (VinzzB)
 * All rights reserved.
 *   
 * This source code is licensed under the Apache 2.0 license found in the
 * LICENSE file in the root directory of this source tree. 
 */
 
#ifndef Product_h
  #define Product_h
  #include "Arduino.h"

  #define PRODUCTNAME_MAX_LEN 15 // 14 chars + null terminator! = 15 chars!
  #define MAX_STEPS 5

  typedef struct CookStep {
    int timeInSec;  //2 bytes
    byte temp;       //1 byte
    bool beep;       //1 byte
  } ;

  typedef struct Product {
    char name[PRODUCTNAME_MAX_LEN];
    bool preHeat;
    byte stepsCount;
    CookStep steps[MAX_STEPS];
  };

#endif
