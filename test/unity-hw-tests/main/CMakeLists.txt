idf_component_register(
    SRCS "test_app_main.cpp"
         "gcov_dumper.cpp"
         "../test_common.cpp"
         "../test_entry_gate_hw.cpp"
         "../test_exit_gate_hw.cpp"
    INCLUDE_DIRS "." ".."
    REQUIRES unity parking_system driver freertos # nvs_flash
)

# Enable coverage only for test files to save IRAM
set_source_files_properties(
    "test_app_main.cpp"
    "../test_entry_gate_hw.cpp"
    "../test_exit_gate_hw.cpp"
    "../test_common.cpp"
    PROPERTIES COMPILE_FLAGS "--coverage"
)
target_link_libraries(${COMPONENT_LIB} PRIVATE --coverage)


# target_compile_definitions(${COMPONENT_LIB} PRIVATE UNITY_INCLUDE_CONFIG_H)
