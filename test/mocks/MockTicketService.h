#pragma once

#include "ITicketService.h"
#include <map>

/**
 * @brief Mock ticket service for testing
 *
 * Controllable ticket service for unit tests.
 */
class MockTicketService : public ITicketService {
public:
    explicit MockTicketService(uint32_t capacity)
        : m_capacity(capacity)
        , m_nextTicketId(1)
    {}

    [[nodiscard]] uint32_t getNewTicket() override {
        uint32_t activeCount = getActiveTicketCount();
        if (activeCount >= m_capacity) {
            return 0;  // Capacity reached
        }

        uint32_t ticketId = m_nextTicketId++;
        m_tickets[ticketId] = Ticket(ticketId, 0);
        return ticketId;
    }

    bool payTicket(uint32_t ticketId) override {
        auto it = m_tickets.find(ticketId);
        if (it == m_tickets.end()) {
            return false;
        }

        it->second.isPaid = true;
        it->second.paymentTimestamp = 123456;  // Mock timestamp
        return true;
    }

    bool validateAndUseTicket(uint32_t ticketId) override {
        auto it = m_tickets.find(ticketId);
        if (it == m_tickets.end() || it->second.isUsed || !it->second.isPaid) {
            return false;
        }

        it->second.isUsed = true;
        return true;
    }

    [[nodiscard]] bool getTicketInfo(uint32_t ticketId, Ticket& ticket) const override {
        auto it = m_tickets.find(ticketId);
        if (it != m_tickets.end()) {
            ticket = it->second;
            return true;
        }
        return false;
    }

    [[nodiscard]] uint32_t getActiveTicketCount() const override {
        uint32_t count = 0;
        for (const auto& [id, ticket] : m_tickets) {
            if (!ticket.isUsed) {
                count++;
            }
        }
        return count;
    }

    [[nodiscard]] uint32_t getCapacity() const override {
        return m_capacity;
    }

    // Test helpers
    void setCapacity(uint32_t capacity) {
        m_capacity = capacity;
    }

    void reset() {
        m_tickets.clear();
        m_nextTicketId = 1;
    }

private:
    uint32_t m_capacity;
    uint32_t m_nextTicketId;
    std::map<uint32_t, Ticket> m_tickets;
};
