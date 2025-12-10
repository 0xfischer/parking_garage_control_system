# Unity Hardware Tests

Hardware-Integrationstests für das Parking Garage Control System.
Diese Tests laufen auf echtem ESP32 oder in Wokwi-Simulation.

## Testdateien

- `test_entry_gate_hw.cpp` - Entry Gate GPIO/Workflow Tests
- `test_exit_gate_hw.cpp` - Exit Gate GPIO/Workflow Tests

## Build & Run

### Lokal mit ESP32 Hardware

```bash
# Unity Test-Firmware bauen
idf.py -T parking_system build

# Flashen und Monitor starten
idf.py -p /dev/ttyUSB0 flash monitor
```

### Mit Wokwi Simulation

```bash
# Bauen und mit Wokwi starten
make test-unity-wokwi

# Oder manuell:
idf.py -T parking_system build
wokwi-cli --timeout 120000 --scenario test/wokwi/unity_hw_tests.yaml
```

## Test-Struktur

Die Tests nutzen ESP-IDF's Unity-Framework. Im Monitor:
- `Press ENTER to see the list of tests` - Test-Menü
- `*` eingeben - Alle Tests ausführen
- `[entry]` - Nur Entry-Gate Tests
- `[exit]` - Nur Exit-Gate Tests

## Hinweis

Der Ordner ist via Symlink mit `components/parking_system/test/` verknüpft,
damit ESP-IDF's `-T` Flag funktioniert.
