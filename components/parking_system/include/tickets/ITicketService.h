#pragma once

#include "Ticket.h"

/**
 * @brief Interface for ticket service
 *
 * Manages parking tickets throughout the vehicle lifecycle:
 * - Entry: Issue new ticket
 * - Payment: Mark ticket as paid
 * - Exit: Validate and use ticket
 */
class ITicketService {
  public:
    virtual ~ITicketService() = default;

    /**
     * @brief Create new ticket
     * @return Ticket ID (0 if failed)
     */
    [[nodiscard]] virtual uint32_t getNewTicket() = 0;

    /**
     * @brief Mark ticket as paid
     * @param ticketId Ticket ID to pay
     * @return true if successful, false if ticket doesn't exist
     */
    virtual bool payTicket(uint32_t ticketId) = 0;

    /**
     * @brief Validate ticket is paid and mark as used (for exit)
     * @param ticketId Ticket ID to validate
     * @return true if paid and not already used, false otherwise
     */
    virtual bool validateAndUseTicket(uint32_t ticketId) = 0;

    /**
     * @brief Get ticket information
     * @param ticketId Ticket ID
     * @param ticket Output ticket structure
     * @return true if ticket exists, false otherwise
     */
    [[nodiscard]] virtual bool getTicketInfo(uint32_t ticketId, Ticket& ticket) const = 0;

    /**
     * @brief Get number of active (not used) tickets
     * @return Number of cars currently in parking garage
     */
    [[nodiscard]] virtual uint32_t getActiveTicketCount() const = 0;

    /**
     * @brief Get maximum parking capacity
     * @return Maximum number of parking spaces
     */
    [[nodiscard]] virtual uint32_t getCapacity() const = 0;

    /**
     * @brief Reset ticket service to initial state
     * Clears all tickets and resets ID counter to 1
     */
    virtual void reset() = 0;
};
