idf_component_register(
    SRCS "src/EntryGateController.cpp"
         "src/ExitGateController.cpp"
    INCLUDE_DIRS "include"
    REQUIRES gpio_hal events tickets freertos
)
