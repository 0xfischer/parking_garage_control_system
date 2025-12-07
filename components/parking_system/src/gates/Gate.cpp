#include "Gate.h"
#include "esp_log.h"

static const char* TAG = "Gate";

Gate::Gate(gpio_num_t lightBarrierPin, gpio_num_t motorPin, ledc_channel_t ledcChannel)
    : m_button(nullptr) // No button
    , m_isOpen(false) {
    // Create light barrier and motor
    m_lightBarrier = std::make_unique<EspGpioInput>(lightBarrierPin, 0);
    m_motor = std::make_unique<EspServoOutput>(motorPin, ledcChannel, false);

    // Start with barrier closed
    m_motor->setLevel(false);
}

Gate::Gate(gpio_num_t buttonPin, uint32_t buttonDebounceMs,
           gpio_num_t lightBarrierPin, gpio_num_t motorPin, ledc_channel_t ledcChannel)
    : m_isOpen(false) {
    // Create button, light barrier and motor
    m_button = std::make_unique<EspGpioInput>(buttonPin, buttonDebounceMs);
    m_lightBarrier = std::make_unique<EspGpioInput>(lightBarrierPin, 0);
    m_motor = std::make_unique<EspServoOutput>(motorPin, ledcChannel, false);

    // Start with barrier closed
    m_motor->setLevel(false);
}

void Gate::open() {
    if (!m_isOpen) {
        ESP_LOGI(TAG, "Opening barrier");
        m_motor->setLevel(true); // HIGH = Open
        m_isOpen = true;
    }
}

void Gate::close() {
    if (m_isOpen) {
        ESP_LOGI(TAG, "Closing barrier");
        m_motor->setLevel(false); // LOW = Closed
        m_isOpen = false;
    }
}

bool Gate::isOpen() const {
    return m_isOpen;
}

bool Gate::isCarDetected() const {
    // Light barrier returns HIGH when blocked (car present)
    return m_lightBarrier->getLevel();
}
