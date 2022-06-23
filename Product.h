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