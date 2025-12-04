idf_component_register(
    SRCS "src/TicketService.cpp"
    INCLUDE_DIRS "include"
    REQUIRES events freertos esp_timer
)
