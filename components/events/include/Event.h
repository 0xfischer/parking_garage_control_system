#pragma once

#include <cstdint>
#include <variant>

/**
 * @brief Event types in the parking system
 */
enum class EventType {
    // Hardware Events (from GPIO interrupts)
    EntryButtonPressed,
    EntryButtonReleased,
    EntryLightBarrierBlocked,
    EntryLightBarrierCleared,
    ExitLightBarrierBlocked,
    ExitLightBarrierCleared,

    // System Events
    CapacityAvailable,
    CapacityFull,
    TicketIssued,
    TicketValidated,
    TicketRejected,

    // State Events (for logging/monitoring)
    EntryBarrierOpened,
    EntryBarrierClosed,
    ExitBarrierOpened,
    ExitBarrierClosed,
    CarEnteredParking,
    CarExitedParking,

    // Timer Events
    BarrierTimeout
};

/**
 * @brief Event payload types
 */
using EventPayload = std::variant<std::monostate, uint32_t, bool>;

/**
 * @brief Event structure
 */
struct Event {
    EventType type;
    uint64_t timestamp;
    EventPayload payload;

    Event() : type(EventType::EntryButtonPressed), timestamp(0), payload(std::monostate{}) {}

    Event(EventType t, uint64_t ts = 0, EventPayload p = std::monostate{})
        : type(t), timestamp(ts), payload(p) {}
};

/**
 * @brief Get string representation of EventType
 */
inline const char* eventTypeToString(EventType type) {
    switch (type) {
        case EventType::EntryButtonPressed: return "EntryButtonPressed";
        case EventType::EntryButtonReleased: return "EntryButtonReleased";
        case EventType::EntryLightBarrierBlocked: return "EntryLightBarrierBlocked";
        case EventType::EntryLightBarrierCleared: return "EntryLightBarrierCleared";
        case EventType::ExitLightBarrierBlocked: return "ExitLightBarrierBlocked";
        case EventType::ExitLightBarrierCleared: return "ExitLightBarrierCleared";
        case EventType::CapacityAvailable: return "CapacityAvailable";
        case EventType::CapacityFull: return "CapacityFull";
        case EventType::TicketIssued: return "TicketIssued";
        case EventType::TicketValidated: return "TicketValidated";
        case EventType::TicketRejected: return "TicketRejected";
        case EventType::EntryBarrierOpened: return "EntryBarrierOpened";
        case EventType::EntryBarrierClosed: return "EntryBarrierClosed";
        case EventType::ExitBarrierOpened: return "ExitBarrierOpened";
        case EventType::ExitBarrierClosed: return "ExitBarrierClosed";
        case EventType::CarEnteredParking: return "CarEnteredParking";
        case EventType::CarExitedParking: return "CarExitedParking";
        case EventType::BarrierTimeout: return "BarrierTimeout";
        default: return "Unknown";
    }
}
