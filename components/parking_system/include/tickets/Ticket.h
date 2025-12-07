#pragma once

#include <cstdint>

/**
 * @brief Parking ticket structure
 */
struct Ticket {
    uint32_t id;
    uint64_t entryTimestamp;
    uint64_t paymentTimestamp; // 0 if not paid
    bool isPaid;
    bool isUsed;

    Ticket()
        : id(0)
        , entryTimestamp(0)
        , paymentTimestamp(0)
        , isPaid(false)
        , isUsed(false) {}

    Ticket(uint32_t ticketId, uint64_t entry)
        : id(ticketId)
        , entryTimestamp(entry)
        , paymentTimestamp(0)
        , isPaid(false)
        , isUsed(false) {}
};
