#include "EspGpioInput.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <cstring>
#include <atomic>

static const char* TAG = "EspGpioInput";

EspGpioInput::EspGpioInput(gpio_num_t pin, uint32_t debounceMs)
    : m_pin(pin)
    , m_debounceMs(debounceMs)
    , m_handler(nullptr)
    , m_lastInterruptTime(0) {
    // Configure GPIO: enable internal pull-up by default for stable input
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << pin);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;      // Use internal pull-up to avoid floating
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // Adjust per sensor if needed
    io_conf.intr_type = GPIO_INTR_ANYEDGE;        // Interrupt on both edges

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO %d: %s", pin, esp_err_to_name(ret));
    } else {
        // Read initial level immediately after configuration
        int initial_level = gpio_get_level(pin);
        ESP_LOGW(TAG, "*** GPIO %d configured, pull: PULLUP, initial level: %d ***", pin, initial_level);
    }
}

EspGpioInput::~EspGpioInput() {
    disableInterrupt();
}

bool EspGpioInput::getLevel() const {
    return gpio_get_level(m_pin) != 0;
}

void EspGpioInput::setInterruptHandler(std::function<void(bool level)> handler) {
    m_handler = std::move(handler);
}

void EspGpioInput::enableInterrupt() {
    if (!m_handler) {
        ESP_LOGW(TAG, "Cannot enable interrupt on GPIO %d: no handler set", m_pin);
        return;
    }

    // Install ISR service if not already installed (done once globally)
    static bool isr_service_installed = false;
    if (!isr_service_installed) {
        esp_err_t ret = gpio_install_isr_service(0);
        if (ret == ESP_OK) {
            isr_service_installed = true;
            ESP_LOGI(TAG, "GPIO ISR service installed");
        } else if (ret != ESP_ERR_INVALID_STATE) { // Already installed
            ESP_LOGE(TAG, "Failed to install ISR service: %s", esp_err_to_name(ret));
            return;
        }
    }

    // Add ISR handler
    esp_err_t ret = gpio_isr_handler_add(m_pin, gpioIsrHandler, this);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add ISR handler for GPIO %d: %s", m_pin, esp_err_to_name(ret));
    } else {
        int current_level = gpio_get_level(m_pin);
        ESP_LOGW(TAG, "*** ISR enabled GPIO %d, level: %d, debounce: %lu ms ***",
                 m_pin, current_level, m_debounceMs);
    }
}

void EspGpioInput::disableInterrupt() {
    gpio_isr_handler_remove(m_pin);
    ESP_LOGI(TAG, "Interrupt disabled on GPIO %d", m_pin);
}

void EspGpioInput::simulateInterrupt(bool level) {
    ESP_LOGI(TAG, "Simulating interrupt on GPIO %d with level %d", m_pin, level);
    if (m_handler) {
        m_handler(level);
    }
}

void IRAM_ATTR EspGpioInput::gpioIsrHandler(void* arg) {
    auto* input = static_cast<EspGpioInput*>(arg);
    if (input) {
        // Simple debug: count interrupts (visible in debugger)
        static std::atomic<uint32_t> isr_count{0};
        isr_count.fetch_add(1, std::memory_order_relaxed);
        input->handleInterrupt();
    }
}

void IRAM_ATTR EspGpioInput::handleInterrupt() {
    // Debug counter
    static std::atomic<uint32_t> handle_count{0};
    handle_count.fetch_add(1, std::memory_order_relaxed);

    // Read current level immediately
    bool level = gpio_get_level(m_pin) != 0;

    // Debouncing: check if enough time has passed since last interrupt
    if (m_debounceMs > 0) {
        int64_t now = esp_timer_get_time();
        int64_t elapsed_ms = (now - m_lastInterruptTime) / 1000;

        if (elapsed_ms < m_debounceMs) {
            // Debug: count debounced interrupts
            static std::atomic<uint32_t> debounce_blocked{0};
            debounce_blocked.fetch_add(1, std::memory_order_relaxed);
            return; // Ignore this interrupt (debounce period)
        }

        m_lastInterruptTime = now;
    }

    // Call handler with current level
    if (m_handler) {
        m_handler(level);
    }
}
