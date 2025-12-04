idf_component_register(
    SRCS "src/ParkingSystem.cpp"
    INCLUDE_DIRS "include"
    REQUIRES gpio_hal events tickets gates freertos
)
