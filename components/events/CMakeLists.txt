idf_component_register(
    SRCS "src/FreeRtosEventBus.cpp"
    INCLUDE_DIRS "include"
    REQUIRES freertos esp_timer
)
