idf_component_register(
    SRCS "test_entry_exit_unity.cpp"
    INCLUDE_DIRS "." "../../../test/mocks"
    REQUIRES gates events tickets gpio_hal unity
)

# Enable UNIT_TEST compile flag so we can use TEST_force* helpers
target_compile_definitions(${COMPONENT_LIB} PRIVATE UNIT_TEST)
