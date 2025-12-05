#pragma once

#include "ParkingGarageConfig.h"

/**
 * @brief Get parking garage system configuration from Kconfig
 *
 * Reads configuration values from Kconfig and creates a ParkingGarageConfig object.
 *
 * @return ParkingGarageConfig Configuration object with values from Kconfig
 */
ParkingGarageConfig get_system_config();
