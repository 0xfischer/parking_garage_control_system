#pragma once

#include "ITicketService.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <map>

/**
 * @brief Thread-safe ticket service implementation
 *
 * Uses FreeRTOS mutex for thread-safety.
 * Stores tickets in memory (no persistence).
 */
class TicketService : public ITicketService {
  public:
    /**
     * @brief Construct ticket service
     * @param capacity Maximum parking capacity
     */
    explicit TicketService(uint32_t capacity);
    ~TicketService() override;

    // Prevent copying
    TicketService(const TicketService&) = delete;
    TicketService& operator=(const TicketService&) = delete;

    [[nodiscard]] uint32_t getNewTicket() override;
    bool payTicket(uint32_t ticketId) override;
    bool validateAndUseTicket(uint32_t ticketId) override;
    [[nodiscard]] bool getTicketInfo(uint32_t ticketId, Ticket& ticket) const override;
    [[nodiscard]] uint32_t getActiveTicketCount() const override;
    [[nodiscard]] uint32_t getCapacity() const override;
    void reset() override;

  private:
    uint32_t m_capacity;
    uint32_t m_nextTicketId;
    std::map<uint32_t, Ticket> m_tickets;
    mutable SemaphoreHandle_t m_mutex;
};
