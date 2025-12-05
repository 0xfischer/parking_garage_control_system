#include "esp32_gpio.h"
#include <iostream>

void Esp32Gpio::setLevel(bool level) {
    // In reality: gpio_set_level(PIN, level);
    currentLevel = level;
    std::cout << "[Esp32Gpio] GPIO set to " << (level ? "HIGH" : "LOW") << "\n";
}

bool Esp32Gpio::getLevel() const {
    return currentLevel;
}
