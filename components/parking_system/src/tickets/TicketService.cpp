#include "TicketService.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char* TAG = "TicketService";

TicketService::TicketService(uint32_t capacity)
    : m_capacity(capacity)
    , m_nextTicketId(1) {
    m_mutex = xSemaphoreCreateMutex();
    if (!m_mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
    }

    ESP_LOGI(TAG, "TicketService created (capacity: %lu)", capacity);
}

TicketService::~TicketService() {
    if (m_mutex) {
        vSemaphoreDelete(m_mutex);
    }
}

uint32_t TicketService::getNewTicket() {
    if (xSemaphoreTake(m_mutex, portMAX_DELAY) == pdTRUE) {
        // Check capacity
        uint32_t activeCount = 0;
        for (const auto& [id, ticket] : m_tickets) {
            if (!ticket.isUsed) {
                activeCount++;
            }
        }

        if (activeCount >= m_capacity) {
            ESP_LOGW(TAG, "Parking full! Cannot issue new ticket (capacity: %lu)", m_capacity);
            xSemaphoreGive(m_mutex);
            return 0; // Capacity reached
        }

        // Create new ticket
        uint32_t ticketId = m_nextTicketId++;
        Ticket ticket(ticketId, esp_timer_get_time());
        m_tickets[ticketId] = ticket;

        ESP_LOGI(TAG, "New ticket issued: ID=%lu (active: %lu/%lu)", ticketId, activeCount + 1, m_capacity);

        xSemaphoreGive(m_mutex);
        return ticketId;
    }

    return 0;
}

bool TicketService::payTicket(uint32_t ticketId) {
    if (xSemaphoreTake(m_mutex, portMAX_DELAY) == pdTRUE) {
        auto it = m_tickets.find(ticketId);
        if (it == m_tickets.end()) {
            ESP_LOGW(TAG, "Ticket not found: ID=%lu", ticketId);
            xSemaphoreGive(m_mutex);
            return false;
        }

        if (it->second.isPaid) {
            ESP_LOGW(TAG, "Ticket already paid: ID=%lu", ticketId);
            xSemaphoreGive(m_mutex);
            return true; // Already paid is not an error
        }

        it->second.isPaid = true;
        it->second.paymentTimestamp = esp_timer_get_time();

        ESP_LOGI(TAG, "Ticket paid: ID=%lu", ticketId);

        xSemaphoreGive(m_mutex);
        return true;
    }

    return false;
}

bool TicketService::validateAndUseTicket(uint32_t ticketId) {
    if (xSemaphoreTake(m_mutex, portMAX_DELAY) == pdTRUE) {
        auto it = m_tickets.find(ticketId);
        if (it == m_tickets.end()) {
            ESP_LOGW(TAG, "Ticket not found: ID=%lu", ticketId);
            xSemaphoreGive(m_mutex);
            return false;
        }

        if (it->second.isUsed) {
            ESP_LOGW(TAG, "Ticket already used: ID=%lu", ticketId);
            xSemaphoreGive(m_mutex);
            return false;
        }

        if (!it->second.isPaid) {
            ESP_LOGW(TAG, "Ticket not paid: ID=%lu", ticketId);
            xSemaphoreGive(m_mutex);
            return false;
        }

        // Mark as used
        it->second.isUsed = true;

        ESP_LOGI(TAG, "Ticket validated and used: ID=%lu", ticketId);

        xSemaphoreGive(m_mutex);
        return true;
    }

    return false;
}

bool TicketService::getTicketInfo(uint32_t ticketId, Ticket& ticket) const {
    if (xSemaphoreTake(m_mutex, portMAX_DELAY) == pdTRUE) {
        auto it = m_tickets.find(ticketId);
        if (it != m_tickets.end()) {
            ticket = it->second;
            xSemaphoreGive(m_mutex);
            return true;
        }
        xSemaphoreGive(m_mutex);
    }

    return false;
}

uint32_t TicketService::getActiveTicketCount() const {
    uint32_t count = 0;

    if (xSemaphoreTake(m_mutex, portMAX_DELAY) == pdTRUE) {
        for (const auto& [id, ticket] : m_tickets) {
            if (!ticket.isUsed) {
                count++;
            }
        }
        xSemaphoreGive(m_mutex);
    }

    return count;
}

uint32_t TicketService::getCapacity() const {
    return m_capacity;
}

void TicketService::reset() {
    if (xSemaphoreTake(m_mutex, portMAX_DELAY) == pdTRUE) {
        m_tickets.clear();
        m_nextTicketId = 1;
        ESP_LOGI(TAG, "TicketService reset: all tickets cleared");
        xSemaphoreGive(m_mutex);
    }
}

void TicketService::setCapacity(uint32_t capacity) {
    if (xSemaphoreTake(m_mutex, portMAX_DELAY) == pdTRUE) {
        m_capacity = capacity;
        ESP_LOGI(TAG, "Capacity set to %lu", capacity);
        xSemaphoreGive(m_mutex);
    }
}
