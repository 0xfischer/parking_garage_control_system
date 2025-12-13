# Development Guide

This guide explains how to set up your development environment, build the project, and test your changes locally.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Development Environment Setup](#development-environment-setup)
- [Building the Project](#building-the-project)
- [Testing](#testing)
- [Code Quality](#code-quality)
- [GitHub Actions Local Testing](#github-actions-local-testing)
- [Devcontainer](#devcontainer)
- [Tips and Tricks](#tips-and-tricks)

## Prerequisites

### Required Tools

- **Docker** - For devcontainer and CI/CD
- **VSCode** (recommended) - With Dev Containers extension
- **Git** - Version control

### Optional Tools

- **ESP-IDF v5.2+** - For native development (not needed with devcontainer)
- **act** - For running GitHub Actions locally
- **gh CLI** - GitHub CLI for workflow management

## Development Environment Setup

### Option 1: Devcontainer (Recommended)

The easiest way to get started is using the devcontainer, which provides a fully configured development environment.

#### 1. Open in VSCode Devcontainer

```bash
# Clone the repository
git clone https://github.com/0xfischer/parking_garage_control_system.git
cd parking_garage_control_system

# Open in VSCode
code .

# VSCode will prompt to "Reopen in Container" - click it
# Or use Command Palette: "Dev Containers: Reopen in Container"
```

#### 2. What's Included in the Devcontainer?

The devcontainer provides:
- ESP-IDF v5.2.2 pre-installed
- All development tools (clang-format, clang-tidy, gcovr, doxygen)
- Wokwi CLI for hardware simulation
- act for local GitHub Actions testing
- pre-commit hooks

### Option 2: Native Development

If you prefer native development without Docker:

```bash
# Install ESP-IDF v5.2+
# Follow: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/

# Set up environment
export IDF_PATH=~/esp/esp-idf
. $IDF_PATH/export.sh

# Install development tools
sudo apt-get install clang-format clang-tidy gcovr doxygen graphviz
```

## Building the Project

### ESP32 Firmware Build

```bash
# Initialize ESP-IDF environment (if not using devcontainer)
make init
# or manually:
. $IDF_PATH/export.sh

# Build the firmware
make build-local

# Clean build
make fullclean
```

The build artifacts are in `build/`:
- `parking_garage_control_system.bin` - Main firmware
- `bootloader/bootloader.bin` - Bootloader
- `partition_table/partition-table.bin` - Partition table

### Flash to ESP32

```bash
# Flash and monitor
idf.py -p /dev/ttyUSB0 flash monitor

# Or use specific commands
idf.py -p /dev/ttyUSB0 flash
idf.py -p /dev/ttyUSB0 monitor
```

## Testing

### Unit Tests (Host Machine)

Unit tests run on your host machine without requiring an ESP32:

```bash
# Run all unit tests
make test-local

# Run specific test
./build-host/test_entry_gate
./build-host/test_exit_gate
./build-host/test_console_workflow
```

### Test Coverage

#### Host Coverage (recommended)
```bash
# Generate coverage report (builds tests with --coverage flags)
make coverage-run

# View coverage report
open build-host/coverage.html
```

**Coverage Details:**
- Host tests use Mocks for HAL/Services → only controller logic is covered
- Current coverage: ~42% overall, ~84% for controllers

**Coverage Reports:**
- `build-host/coverage.html` - HTML report with line-by-line details

#### ESP32 Coverage (requires JTAG hardware)

**Important**: ESP32 gcov coverage requires JTAG/OpenOCD hardware.
The gcov runtime (`__gcov_merge_add` etc.) is not available for Xtensa
cross-compilation. Wokwi simulation cannot collect coverage data.

For ESP32 coverage with JTAG, see:
- [ESP-IDF Gcov Guide](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/app_trace.html#gcov-source-code-coverage)
- Example: `/opt/esp/idf/examples/system/gcov/`

### Wokwi Hardware Simulation

Wokwi provides hardware-in-the-loop simulation:

```bash
# Single test (entry_exit_flow)
make test-wokwi

# All tests
make test-wokwi-full

# Specific scenario with timeout
wokwi-cli --scenario test/wokwi-tests/console_full.yaml --timeout 60000
```

#### Available Test Scenarios

| Scenario | Description | Status |
|----------|-------------|--------|
| `console_full.yaml` | Entry + payment + exit via console | ✅ Working |
| `entry_exit_flow.yaml` | Hardware button + light barrier | ✅ Working |
| `parking_full.yaml` | Capacity rejection test | ⚠️ Needs longer timeout |

**Note:** Wokwi CLI is located at `/home/develop/bin/wokwi-cli` in the devcontainer.

### Unity Hardware Tests (ESP32)

Unity tests for real hardware validation:

```bash
# Tests location
components/parking_system/test/
├── test_entry_gate_hw.cpp   # Entry gate GPIO tests
├── test_exit_gate_hw.cpp    # Exit gate GPIO tests
└── CMakeLists.txt
```

These tests verify actual GPIO interactions and run on:
- Real ESP32 hardware
- Wokwi simulation

**Test Cases:**
- Entry button triggers state change
- Ticket issuance on entry
- Complete entry/exit cycles
- Parking full rejection
- Unpaid ticket rejection

## Code Quality

### Format Checking

```bash
# Check formatting
make format-check

# Auto-format code
make format
```

### Linting with clang-tidy

```bash
# Generate compile_commands.json
make lint-tidy-db

# Run clang-tidy on all files
make lint-tidy

# Run clang-tidy on changed files only
make lint-tidy-changed
```

### Pre-commit Hooks

The project uses pre-commit hooks for automatic code quality checks:

```bash
# Install pre-commit hooks
pre-commit install

# Run manually
pre-commit run --all-files
```

**Pre-commit Hook:** Runs `clang-format` check on staged C/C++ files
**Pre-push Hook:** Runs `clang-tidy` on changed files (disabled by default)

To enable clang-tidy on push:
```bash
ENABLE_TIDY=1 git push
```

## GitHub Actions Local Testing

### Using the GitHub Local Actions Extension

VSCode extension: [GitHub Local Actions](https://marketplace.visualstudio.com/items?itemName=SanjulaGanepola.github-local-actions)

1. Install the extension in VSCode
2. Open Command Palette (`Ctrl+Shift+P`)
3. Run: "GitHub Local Actions: Run Workflow"
4. Select the workflow you want to test

### Using act CLI

`act` allows you to run GitHub Actions workflows locally using Docker:

```bash
# Run all workflows
make act-test

# Run specific job
make act-build    # Build job
make act-format   # Format check
make act-lint     # Lint check

# Run specific workflow
act -j build
act -j coverage
act -j format
```

#### Running Wokwi Tests with act

Wokwi tests require a license token:

```bash
# Create .env file (not committed)
make env-example
echo "WOKWI_CLI_TOKEN=your_token_here" >> .env

# Run Wokwi tests
make act-wokwi
```

### Manual Workflow Triggers

All workflows support manual triggering via GitHub Actions UI:

1. Go to **Actions** tab on GitHub
2. Select the workflow (Build, Coverage, Format, Tidy, Docs)
3. Click **"Run workflow"**
4. Select branch and click **"Run workflow"**

Or use GitHub CLI:
```bash
# Build on current branch
gh workflow run build.yml

# Build on specific branch
gh workflow run build.yml --ref feature/my-feature

# Other workflows
gh workflow run coverage.yml
gh workflow run format.yml
gh workflow run tidy.yml
gh workflow run wokwi-tests.yml
```

### Managing Workflow Runs

Delete failed workflow runs to keep your Actions history clean:

```bash
# Delete all failed runs
gh run list --status=failure --json databaseId --jq '.[].databaseId' | \
  xargs -I{} gh run delete {}

# Delete failed runs for specific workflow
gh run list --workflow=build.yml --status=failure --json databaseId --jq '.[].databaseId' | \
  xargs -I{} gh run delete {}

# Delete all failed runs across all workflows in this project
for workflow in build.yml coverage.yml format.yml tidy.yml docs.yml wokwi-tests.yml release.yml
do
  echo "Deleting failed runs for $workflow..."
  gh run list --workflow=$workflow --status=failure --json databaseId --jq '.[].databaseId' | \
    xargs -I{} gh run delete {} 2>/dev/null || true
done

# List failed runs before deleting (to review)
gh run list --status=failure --json databaseId,displayTitle,conclusion

# Delete with limit (e.g., last 50 failed runs)
gh run list --status=failure --limit 50 --json databaseId --jq '.[].databaseId' | \
  xargs -I{} gh run delete {}
```

**Note:** Deleted workflow runs cannot be recovered. The logs and artifacts are permanently removed.

## Devcontainer

### Devcontainer Image

The project uses a custom Docker image: `ghcr.io/0xfischer/parking_garage_control_system-dev:latest`

**Image includes:**
- ESP-IDF v5.2.2
- Build tools (cmake, ninja, gcc, g++)
- Code quality tools (clang-format, clang-tidy, gcovr, lcov)
- Documentation tools (doxygen, graphviz)
- Testing tools (wokwi-cli, act)
- Pre-commit hooks

### Rebuilding the Devcontainer Image

```bash
# Build and push new image
make docker-release TAG=latest

# Build with specific ESP-IDF version
make docker-release TAG=v1.0.0 ESP_IDF_VERSION=v5.2.2
```

### Devcontainer Configuration

Configuration is in `.devcontainer/`:
- `Dockerfile` - Image definition
- `devcontainer.json` - VSCode devcontainer settings

**Key environment variables:**
- `IDF_PATH=/opt/esp/idf` - ESP-IDF installation path
- `IDF_TOOLS_PATH=/opt/esp` - ESP-IDF tools path
- `TERM=xterm-256color` - Color terminal support

**ESP-IDF in Devcontainer:**
```bash
# Source ESP-IDF environment
export IDF_PATH=/opt/esp/idf
source $IDF_PATH/export.sh

# Or use idf.py directly
python /opt/esp/idf/tools/idf.py build

# Wokwi CLI location
/home/develop/bin/wokwi-cli --version
```

### Accessing Hardware in Devcontainer

The devcontainer is configured with `--privileged` and mounts:
- `/dev` - For USB device access (ESP32 flashing)
- `/var/run/docker.sock` - For Docker-in-Docker (act, etc.)

## Tips and Tricks

### Quick Commands

```bash
# Complete CI pipeline locally
make ci-local

# Clean everything
make fullclean

# Initialize ESP-IDF environment
make init
```

### VSCode Tasks

The project includes VSCode tasks (`.vscode/tasks.json`):
- **Build** - `Ctrl+Shift+B`
- **Flash** - Flash firmware to ESP32
- **Monitor** - Serial monitor
- **Clean** - Clean build artifacts

### Serial Monitor

```bash
# Using idf.py
idf.py monitor

# Using screen
screen /dev/ttyUSB0 115200

# Using minicom
minicom -D /dev/ttyUSB0 -b 115200
```

### Debugging

```bash
# Enable verbose build
idf.py build -v

# ESP-IDF menuconfig
idf.py menuconfig

# Component config
idf.py menuconfig
# Navigate to: Component config -> Parking System
```

### Git Workflow

```bash
# Create feature branch
git checkout -b feature/my-feature

# Make changes and commit
git add .
git commit -m "feat: Add new feature"

# Push and create PR
git push -u origin feature/my-feature
gh pr create
```

### Troubleshooting

**Problem:** `idf.py: command not found`
**Solution:** Run `make init` or `. $IDF_PATH/export.sh`

**Problem:** Git commit fails with format errors
**Solution:** Run `make format` to auto-format code

**Problem:** USB device not accessible
**Solution:** Add user to `dialout` group: `sudo usermod -aG dialout $USER`

**Problem:** Docker permission denied
**Solution:** Add user to `docker` group: `sudo usermod -aG docker $USER`

**Problem:** act fails to pull image
**Solution:** Login to GitHub Container Registry: `echo $GITHUB_TOKEN | docker login ghcr.io -u USERNAME --password-stdin`

## Additional Resources

- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [Wokwi Documentation](https://docs.wokwi.com/)
- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [act Documentation](https://github.com/nektos/act)
- [Project README](README.md) - Project overview and architecture
- [CLAUDE.md](CLAUDE.md) - Claude Code AI assistant guidelines

## Support

For issues and questions:
- Open an issue on GitHub
- Check existing issues for solutions
- Review GitHub Actions logs for CI failures

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run tests and quality checks locally
5. Create a Pull Request
6. Ensure CI passes

See [CONTRIBUTING.md](CONTRIBUTING.md) for detailed contribution guidelines.
