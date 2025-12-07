#include "EspGpioOutput.h"
#include "esp_log.h"

static const char* TAG = "EspGpioOutput";

EspGpioOutput::EspGpioOutput(gpio_num_t pin, bool initialLevel)
    : m_pin(pin)
    , m_currentLevel(initialLevel) {
    // Configure GPIO as output
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << pin);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO %d: %s", pin, esp_err_to_name(ret));
    } else {
        // Set initial level
        gpio_set_level(m_pin, initialLevel ? 1 : 0);
        ESP_LOGI(TAG, "GPIO %d configured as output (initial: %s)", pin, initialLevel ? "HIGH" : "LOW");
    }
}

void EspGpioOutput::setLevel(bool high) {
    m_currentLevel = high;
    gpio_set_level(m_pin, high ? 1 : 0);
}

bool EspGpioOutput::getLevel() const {
    return m_currentLevel;
}
