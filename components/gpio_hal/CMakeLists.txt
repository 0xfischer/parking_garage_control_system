idf_component_register(
    SRCS "src/EspGpioInput.cpp"
         "src/EspGpioOutput.cpp"
         "src/EspServoOutput.cpp"
    INCLUDE_DIRS "include"
    REQUIRES driver hal esp_timer
)
