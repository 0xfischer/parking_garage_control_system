# Copilot Instructions – Parking Garage Control System

Use these repo-specific notes to be productive fast. Stick to the patterns below and cite the files you change.

## Architecture (what matters here)
- Event-driven system on ESP32 (ESP-IDF + FreeRTOS). Core pieces:
  - `components/parking_system/include/events/` → `Event.h`, `IEventBus.h`, `FreeRtosEventBus` (queue-backed pub/sub)
  - `components/parking_system/include/gates/` → `EntryGateController`, `ExitGateController`, `Gate` (state machines + HAL use)
  - `components/parking_system/include/hal/` → `IGpioInput`, `IGpioOutput`, `EspGpioInput`, `EspServoOutput` (LEDC)
  - `components/parking_system/include/tickets/` → `TicketService` (issue/pay/validate)
  - `components/parking_system/include/parking/` → `ParkingGarageSystem` (wires everything and sets up ISRs)
- Data flow: GPIO ISR → publish `Event` → controller state machine transitions → gate motor open/close.
- Important conventions:
  - Light barrier logic: LOW = blocked (car). Prefer `IGate::isCarDetected()` over raw levels (see README “Common Issues”).
  - Dual constructors: production creates hardware; tests inject mocks. Keep this separation.

## Developer workflows
- Build/flash (ESP-IDF): use VS Code tasks or `idf.py`.
  - Tasks: “Build - IDF”, “Flash - IDF”, “Monitor - IDF”, “Build + Flash + Monitor”.
- Wokwi simulation scenarios: `test/wokwi-tests/*.yaml` (e.g., `entry_exit_flow.yaml`, `console_full.yaml`).
- Host unit tests (mocks) live under `test/unit-tests/`; build with `-DUNIT_TEST` and includes in `test/stubs/` and `test/mocks/`. Coverage: `make coverage-run` → `build-host/coverage.html`.

## Unity hardware tests (ESP32)
- Location: `test/unity-hw-tests/` (e.g., `test_exit_gate_hw.cpp`, `test_entry_gate_hw.cpp`).
- Runner: `test/unity-hw-tests/main/test_app_main.cpp` must call `unity_run_tests()` (or `unity_run_tests_by_tag()`), not `unity_run_all_tests()`.
- Registration: use ESP-IDF’s `TEST_CASE("name", "[tag]") { ... }`. Do not mix with `RUN_TEST()`.
- CMake (example): ensure all test sources are listed and `REQUIRES unity` (see that folder’s `CMakeLists.txt`). Avoid defining `UNITY_INCLUDE_CONFIG_H` unless you provide a matching `unity_config.h`.
- Useful helpers from `test_common.h/.cpp`: GPIO constants and faster timeouts (e.g., `barrierTimeoutMs = 500`). Use `simulateInterrupt()` on `EspGpioInput` to trigger ISR paths.

## Patterns to follow when changing code
- Controllers: prefer publishing/handling `Event` over directly poking hardware; keep state transitions explicit and testable.
- New hardware interactions: add an interface in `hal/`, implement `Esp*` for production, and consider a mock in `test/mocks/` for host tests.
- Console commands (`main/console_commands.cpp`): route to `ParkingGarageSystem` or services; mirror behaviors in `test/test_console_workflow.cpp`.
- ISR setup belongs to `ParkingGarageSystem`; controllers consume events. Don’t block in ISRs; push to EventBus.

## Gotchas (seen in this repo)
- Unity: “0 Tests” usually means tests weren’t built into the test component or `UNITY_INCLUDE_CONFIG_H` was defined without a `unity_config.h`. Fix CMake or drop that define.
- Light barrier polarity: many bugs come from inverting this; rely on `Gate::isCarDetected()`.
- Timing in tests: shorten via injected constructor params (e.g., validation/open/close timeouts) to keep tests fast.

## Small examples
- Add a Unity test by tag (exit gate):
  ```cpp
  TEST_CASE("exit rejects unpaid ticket", "[exit_gate]") {
    auto& sys = get_test_system();
    auto& tickets = sys.getTicketService();
    auto& exitCtrl = sys.getExitGate();
    uint32_t id = tickets.getNewTicket();
    TEST_ASSERT_FALSE(exitCtrl.validateTicketManually(id));
  }
  ```
- Publish an event in tests:
  ```cpp
  Event e(EventType::EntryButtonPressed);
  sys.getEventBus().publish(e);
  ```

## Where to look first
- Overview and state machines: `README.md` (mermaid diagrams).
- Test how-tos: `test/README.md`, `test/unity-hw-tests/README.md`.
- Wokwi wiring: `diagram.json`, `wokwi.toml`, `test/wokwi-tests/*.yaml`.
