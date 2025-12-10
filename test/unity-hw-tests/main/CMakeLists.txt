idf_component_register(
    SRCS "test_app_main.cpp"
         "../test_entry_gate_hw.cpp"
         "../test_exit_gate_hw.cpp"
    INCLUDE_DIRS "." ".."
    REQUIRES unity parking_system driver freertos
)
