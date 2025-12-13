# Test Coverage Report

## Übersicht

| Test-Typ | Umgebung | Coverage | Status |
|----------|----------|----------|--------|
| Host Unit Tests | g++ (Host) | 42% | ✅ Erfolgreich |
| Wokwi Simulation | ESP32 Emulation | N/A* | ✅ 2/3 Tests |
| Unity HW Tests | ESP32 Hardware | N/A* | ⚠️ Implementiert |

*Coverage für ESP32-Code nicht direkt messbar (Cross-Compilation)

## Host-Tests Coverage Details

### Gut abgedeckt (>80%)
- `EntryGateController.cpp`: **84%** (121/143 Zeilen)
- `ExitGateController.cpp`: **81%** (122/149 Zeilen)

### Nicht abgedeckt (Mocks verwendet)
- `FreeRtosEventBus.cpp`: 0% (MockEventBus verwendet)
- `TicketService.cpp`: 0% (MockTicketService verwendet)
- `Gate.cpp`: 0% (MockGate verwendet)
- `EspGpioInput.cpp`: 0% (MockGpioInput verwendet)
- `EspServoOutput.cpp`: 0% (Hardware-Mocks)

## Wokwi-Tests

| Szenario | Status | Beschreibung |
|----------|--------|--------------|
| console_full.yaml | ✅ Bestanden | Entry + Payment + Exit via Console |
| entry_exit_flow.yaml | ✅ Bestanden | Hardware-Button + Light Barrier Flow |
| parking_full.yaml | ⚠️ Timeout | Kapazitätstest (benötigt längeres Timeout) |

## Unity Hardware Tests

Die folgenden Tests sind in `components/parking_system/test/` implementiert:

### Entry Gate Tests (`test_entry_gate_hw.cpp`)
- Entry button triggers CheckingCapacity state
- Entry flow issues ticket
- Complete entry cycle
- Parking full rejects entry

### Exit Gate Tests (`test_exit_gate_hw.cpp`)
- Exit rejects unpaid ticket
- Exit accepts paid ticket
- Complete exit cycle with light barrier

## Empfehlungen

1. **Erhöhe Host-Test Coverage**: Tests für TicketService, FreeRtosEventBus und Gate mit echten Implementierungen statt Mocks
2. **Wokwi parking_full Test**: Timeout erhöhen oder Delays reduzieren
3. **Unity Tests in CI**: `pytest-embedded` oder ähnlich für ESP32 Unity Tests

## Coverage-Dateien

- `coverage/combined_coverage.html` - Detaillierter HTML-Report
- `coverage/coverage.xml` - XML-Report für CI-Integration
- `coverage/host/` - Vorherige Host-Coverage-Daten
