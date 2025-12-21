# Unity Hardware Tests

Hardware-Integrationstests für das Parking Garage Control System.
Diese Tests laufen auf echtem ESP32 oder in Wokwi-Simulation.

## Testdateien

- `test_entry_gate_hw.cpp` - Entry Gate Tests (6 Tests)
- `test_exit_gate_hw.cpp` - Exit Gate Tests (4 Tests)
- `test_common.cpp/.h` - Gemeinsame Test-Infrastruktur
- `test_app_main.cpp` - Test-Runner (main)

## Test-Typen

### EventBus-gesteuerte Tests
Portable Tests die Events über den EventBus publizieren:
```cpp
Event buttonEvent(EventType::EntryButtonPressed);
eventBus.publish(buttonEvent);
```

### GPIO-gesteuerte Tests
Tests die den ISR-Handler direkt aufrufen via `simulateInterrupt()`:
```cpp
gate.getButton().simulateInterrupt(GPIO_BUTTON_PRESSED);
gate.getLightBarrier().simulateInterrupt(GPIO_LIGHT_BARRIER_BLOCKED);
```

## GPIO Simulation Constants

Definiert in `test_common.h`:
```cpp
GPIO_LIGHT_BARRIER_BLOCKED  // false (LOW) = Auto erkannt
GPIO_LIGHT_BARRIER_CLEARED  // true (HIGH) = kein Auto
GPIO_BUTTON_PRESSED         // false (LOW) = gedrückt (active low)
GPIO_BUTTON_RELEASED        // true (HIGH) = losgelassen
```

## Test-Konfiguration

In `test_common.cpp`:
- `barrierTimeoutMs = 500` (schnellere Tests)
- `capacity = 3` (um "Parkhaus voll" zu testen)

## Build & Run

### Mit Wokwi Simulation

```bash
cd test/unity-hw-tests
idf.py build
wokwi-cli --timeout 120000 --scenario ../wokwi-tests/unity_hw_tests.yaml
```

### Lokal mit ESP32 Hardware

```bash
cd test/unity-hw-tests
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Coverage Generation

Automated coverage generation using QEMU (Default) or Wokwi.

### Default (QEMU)
Runs the tests in QEMU and checks coverage locally. No external account required.

```bash
python3 ../../tools/run_coverage.py
```

### Wokwi (Optional)
Runs the tests in Wokwi simulator (requires Wokwi CLI token).

```bash
python3 ../../tools/run_coverage.py --wokwi
```

The script will:
1. Build the project with coverage flags.
2. Run simulation (QEMU for ~30s, or Wokwi).
3. Extract coverage data from console output.
4. Generate an HTML report in `coverage_report/index.html`.


## Test-Ablauf

Die Tests starten automatisch nach dem Boot:
1. System-Initialisierung mit Test-Konfiguration
2. `setUp()` wird vor jedem Test aufgerufen (reset)
3. Alle Tests laufen sequenziell
4. Ergebnis: `10 Tests 0 Failures`

## Entry Gate Tests (6)

1. `test_complete_entry_cycle` - Kompletter Einfahrt-Zyklus
2. `test_second_entry_cycle` - Zweite Einfahrt funktioniert
3. `test_button_ignored_when_busy` - Button wird ignoriert wenn beschäftigt
4. `test_gpio_button_triggers_entry` - GPIO Button triggert Einfahrt
5. `test_gpio_light_barrier_detects_car` - GPIO Lichtschranke erkennt Auto
6. `test_parking_full_rejects_entry` - Parkhaus voll lehnt Einfahrt ab

## Exit Gate Tests (4)

1. `test_exit_rejects_unpaid_ticket` - Unbezahltes Ticket wird abgelehnt
2. `test_exit_accepts_paid_ticket` - Bezahltes Ticket wird akzeptiert
3. `test_complete_exit_cycle` - Kompletter Ausfahrt-Zyklus
4. `test_gpio_exit_light_barrier` - GPIO Lichtschranke bei Ausfahrt
