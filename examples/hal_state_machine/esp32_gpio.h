#ifndef ESP32_GPIO_H
#define ESP32_GPIO_H

#include "hal_state_machine.h"

// Concrete Implementation for ESP32 GPIO
class Esp32Gpio : public IGpioOutput {
public:
    void setLevel(bool level) override;
    bool getLevel() const override;

private:
    bool currentLevel = false;
};

#endif // ESP32_GPIO_H
