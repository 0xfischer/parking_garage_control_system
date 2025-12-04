idf_component_register(
    SRCS "main.cpp"
         "console_commands.cpp"
    INCLUDE_DIRS "."
    REQUIRES gpio_hal events tickets gates parking console nvs_flash
)
