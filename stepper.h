#include <Arduino.h>

#define GS_MIN_US 300000  // период, длиннее которого мотор можно резко тормозить или менять скорость

#include <Arduino.h>


#ifndef DRIVER_STEP_TIME
#define DRIVER_STEP_TIME 50
#endif


class Stepper {
   private:
    // настройка пина
    void configurePin(int num, uint8_t pin) {
        pinMode(pin, OUTPUT);
        _pins[num] = pin;
    }

    // быстрая установка пина
    void setPin(int num, bool state) {
        digitalWrite(_pins[num], state);
    }

    
    // шажочек степдир
    void stepDir() {
            if (_pdir != dir) {
                _pdir = dir;
                setPin(1, (dir > 0) ^ _globDir);  // DIR
            }
            setPin(0, 1);  // step HIGH
            if (DRIVER_STEP_TIME > 0) delayMicroseconds(DRIVER_STEP_TIME);
            setPin(0, 0);  // step LOW

    }

    int8_t _enPin = 255;
    bool _enDir = false;
    bool _globDir = false;
    int8_t _pdir = 0;
    int8_t thisStep = 0;

    void (*_step)(uint8_t a) = NULL;
    void (*_power)(bool a) = NULL;

    uint8_t _pins[2];


   public:
    Stepper(uint8_t pin1 = 255, uint8_t pin2 = 255, uint8_t pin3 = 255, uint8_t pin4 = 255, uint8_t pin5 = 255) {

                configurePin(0, pin1);
                configurePin(1, pin2);
                if (pin3 != 255) {
                    _enPin = pin3;
                    pinMode(_enPin, OUTPUT);
                }

        }
    

    // сделать шаг
    void step() {
        pos += dir;    
        stepDir();
    }

    // инвертировать поведение EN пина
    void invertEn(bool val) {
        _enDir = val;
    }

    // инвертировать направление мотора
    void reverse(bool val) {
            if (_globDir != val) setPin(1, (dir > 0) ^ val);
        _globDir = val;
    }

    // отключить питание и EN
    void disable() {
            if (_enPin != 255) digitalWrite(_enPin, !_enDir);
    }

    // включить питание и EN
    void enable() {
            if (_enPin != 255) digitalWrite(_enPin, _enDir);
        
    }

    // переключить питание
    void power(bool state) {
        if (state) enable();
        else disable();
    }

    // подключить обработчик шага
    void attachStep(void (*handler)(uint8_t)) {
        _step = handler;
    }

    // подключить обработчик питания
    void attachPower(void (*handler)(bool)) {
        _power = handler;
    }

    int32_t pos = 0;
    int8_t dir = 1;


};

class GStepper2 : public Stepper {
   public:
    // ========================= КОНСТРУКТОР ==========================
    GStepper2(uint16_t steps, uint8_t pin1 = 255, uint8_t pin2 = 255, uint8_t pin3 = 255, uint8_t pin4 = 255, uint8_t pin5 = 255) : Stepper(pin1, pin2, pin3, pin4, pin5) {
        stepsRev = steps;
        setMaxSpeed(100);
        setAcceleration(200);
    }

    // ============================= TICK =============================
    // тикер. Вернёт true, если мотор движется
    bool tick() {
        if (status) {
            uint32_t thisUs = micros();
            if (thisUs - tmr >= us) {
                tmr = thisUs;
                tickManual();
            }
        }
        return status;
    }

    // ручной тикер для вызова в прерывании таймера. Вернёт true, если мотор движется
    bool tickManual() {
        if (!status) return 0;  // стоим-выходим
        step();                 // шаг

        if (status <= 2 && pos == tar) {
            if (status == 1) readyF = 1;
            brake();
        }
        return status;
    }

    // ============================= SPEED MODE =============================
    // установить скорость вращения
    bool setSpeed(int32_t speed) {
        if (speed == 0) {
            brake();
            return 0;
        }
        dir = (speed > 0) ? 1 : -1;
        us = 1000000L / abs(speed);
        status = 3;
        if (autoP) enable();
        return 1;
    }


    // установить скорость вращения float
    void setSpeed(double speed) {
        if (setSpeed((int32_t)speed)) us = 1000000.0 / abs(speed);
    }

    void setSpeedDeg(int speed) {
        setSpeed((int32_t)speed * stepsRev / 360L);
    }

    void setSpeedDeg(double speed) {
        setSpeed((float)speed * stepsRev / 360L);
    }

    // =========================== POSITION MODE ===========================
    // установить цель и опционально режим
    void setTarget(int32_t ntar) {
        if (sp0) {  // нулевая скорость
            brake();
            readyF = 1;
            return;
        }

        if (changeSett) {  // применяем настройки
            usMin = usMinN;

            changeSett = 0;
        }

        tar = ntar;

        if (tar == pos) {
            brake();
            readyF = 1;
            return;
        }


        us = usMin;

        dir = (pos < tar) ? 1 : -1;
        status = 1;
        if (autoP) enable();
        readyF = 0;
    }



    // получить целевую позицию
    int32_t getTarget() {

        return tar;

    }

    // установить текущую позицию
    void setCurrent(int32_t npos) {
        pos = npos;
    }

    // получить текущую позицию
    int32_t getCurrent() {
        return pos;
    }

    // сбросить текущую позицию в 0
    void reset() {
        pos = 0;
    }

    // ========================== POSITION SETTINGS ==========================
    // установка ускорения в шаг/сек^2
    void setAcceleration(uint16_t acc) {

    }

    // установить скорость движения при следовании к позиции в шагах/сек
    void setMaxSpeed(double speed) {
        if (speed == 0) {
            sp0 = 1;
            return;
        }
        sp0 = 0;
        usMinN = 1000000.0 / speed;
        if (!status) {  // применяем, если мотор остановлен
            usMin = usMinN;

            changeSett = 0;
        } else changeSett = 1;  // иначе флаг на изменение
    }

    // установить скорость движения при следовании к позиции в град/сек, float
    void setMaxSpeedDeg(double speed) {
        setMaxSpeed(speed * stepsRev / 360.0);
    }

    // =========================== PLANNER ============================
    // остановить плавно (с заданным ускорением)
    void stop() {

        brake();
    }

    // автоотключение мотора при достижении позиции - true (по умолч. false)
    void autoPower(bool mode) {
        autoP = mode;
    }

    // остановить мотор
    void brake() {
        status = 0;
        if (autoP) disable();
    }

    // пауза (доехать до заданной точки и ждать). ready() не вернёт true, пока ты на паузе
    void pause() {
        if (status == 1) status = 2;
    }

    // продолжить движение после остановки
    void resume() {
        if (!status) setTarget(tar);
    }

    // текущий статус: 0 - стоим, 1 - едем, 2 - едем к точке паузы, 3 - крутимся со скоростью, 4 - тормозим
    uint8_t getStatus() {
        return status;
    }

    // вернёт true, если мотор доехал до установленной позиции и остановился
    bool ready() {
        if (!status && readyF) {
            readyF = 0;
            return 1;
        }
        return 0;
    }

    // получить текущий период вращения
    uint32_t getPeriod() {
        return us;
    }

    // чики пуки
    using Stepper::pos;
    using Stepper::dir;
    using Stepper::step;
    using Stepper::enable;
    using Stepper::disable;

    // ============================= PRIVATE =============================
   private:
    void calcPlan() {

    }

    uint32_t tmr = 0, us = 10000, usMin = 10000;
    int32_t tar = 0;
    uint16_t stepsRev;
    uint8_t status = 0;
    bool readyF = 0;
    bool changeSett = 0;
    bool autoP = false;
    uint32_t usMinN;
    bool sp0 = 0;


    uint32_t prfS[GS_FAST_PROFILE], prfP[GS_FAST_PROFILE];
};