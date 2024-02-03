#ifndef LIFT_H
#define LIFT_H

#include "stepper.h"
// #include "GStypes.h"

// #define GS_FAST_PROFILE 10


enum LiftDir{LIFTUP, LIFTMOVEUP,LIFTDOWN, LIFTMOVEDOWN};


template <GS_driverType _DRV, GS_driverType _TYPE = STEPPER_PINS>
class Lift : public GStepper2<_DRV, _TYPE> {




    LiftDir lift_dir =LIFTMOVEDOWN;
    uint8_t stoppin;
     public:
    // ========================= КОНСТРУКТОР ==========================

   public:
    // ========================= КОНСТРУКТОР ==========================
    Lift(uint8_t _stoppin , uint16_t steps=2048, uint8_t pin1 = 255, uint8_t pin2 = 255, uint8_t pin3 = 255, uint8_t pin4 = 255, uint8_t pin5 = 255) : GStepper2<_DRV, _TYPE>(steps , pin1, pin2, pin3, pin4, pin5) {
      stoppin = _stoppin;
    }



    // Lift(uint8_t _stoppin = 4, uint16_t steps = 2048, uint8_t pin1 = 255, uint8_t pin2 = 255, uint8_t pin3 = 255,uint8_t pin4 = 255): Stepper<_DRV, _TYPE>( pin1, pin2, pin3, pin4) {
    //   stoppin = stoppin;

    // }

    bool tick () {

    int r = analogRead(stoppin);


    // Serial.println(r);

     if ( r < 200 && lift_dir==LIFTMOVEDOWN) {
       lift_dir = LIFTDOWN;
       brake();
       reset();
       power(false);
     }

   // если приехали
     if (ready() && lift_dir == LIFTMOVEUP) {
       lift_dir = LIFTUP;
       brake();
       reset();
       power(false);
     }

    return GStepper2<_DRV, _TYPE>::tick();
    }

    bool up () {

        Serial.print("up");
        Serial.println(lift_dir );

        if (ready())
          return;
         
        if (lift_dir == LIFTDOWN ) {
          lift_dir = LIFTMOVEUP;
          reset();
          setTarget(110000L);
          power(true);


        }
    }

    bool down () {

        Serial.print("down");

        Serial.println(lift_dir );
 
        if (ready())
          return;
        if ( lift_dir ==LIFTUP ) {


          lift_dir = LIFTMOVEDOWN;
          reset();
          setTarget(-120000L);
          power(true);
        }
    }

};

#endif