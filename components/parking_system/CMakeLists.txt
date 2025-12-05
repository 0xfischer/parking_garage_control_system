idf_component_register(
    SRCS
        # HAL sources
        "src/hal/EspGpioInput.cpp"
        "src/hal/EspGpioOutput.cpp"
        "src/hal/EspServoOutput.cpp"

        # Event system sources
        "src/events/FreeRtosEventBus.cpp"

        # Ticket service sources
        "src/tickets/TicketService.cpp"

        # Gate controllers sources
        "src/gates/Gate.cpp"
        "src/gates/EntryGateController.cpp"
        "src/gates/ExitGateController.cpp"

        # Parking system sources
        "src/parking/ParkingSystem.cpp"

    INCLUDE_DIRS
        "include"
        "include/hal"
        "include/events"
        "include/tickets"
        "include/gates"
        "include/parking"

    REQUIRES
        driver      # For GPIO, LEDC
        freertos    # For FreeRTOS
        esp_timer   # For esp_timer
)
