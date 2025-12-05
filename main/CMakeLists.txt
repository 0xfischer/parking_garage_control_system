idf_component_register(
    SRCS "main.cpp"
         "console_commands.cpp"
    INCLUDE_DIRS "."
    REQUIRES parking_system console nvs_flash
)
