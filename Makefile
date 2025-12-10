-include .env
export

.PHONY: test-local build-local format-check lint-check coverage-run act-test fullclean lint-tidy-db lint-tidy lint-tidy-changed wokwi-test wokwi-test-ci \
	env-print env-example act-wokwi docker-release init test-wokwi-coverage build-coverage build-unity-tests test-unity-wokwi

JOBS := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)

init:
	@if [ -z "$$IDF_PATH" ]; then \
		echo "Error: IDF_PATH not set. Please set it to your ESP-IDF installation path."; \
		echo "Example: export IDF_PATH=~/esp/esp-idf"; \
		exit 1; \
	fi; \
	echo "Sourcing ESP-IDF environment from $$IDF_PATH/export.sh..."; \
	. $$IDF_PATH/export.sh && echo "ESP-IDF environment initialized successfully"

fullclean:
	@bash -c ". $${IDF_PATH:-/opt/esp/idf}/export.sh && idf.py fullclean" || true
	rm -rf build sdkconfig || true

build-local:
	@bash -c ". $${IDF_PATH:-/opt/esp/idf}/export.sh && idf.py build"

test-local:
	cmake -S test -B build-host -DCMAKE_BUILD_TYPE=Debug
	cmake --build build-host
	ctest --test-dir build-host --output-on-failure
test-wokwi:
	wokwi-cli --scenario test/wokwi-tests/entry_exit_flow.yaml
test-wokwi-full:
	wokwi-cli --scenario test/wokwi-tests/console_full.yaml
	wokwi-cli --scenario test/wokwi-tests/entry_exit_flow.yaml
	wokwi-cli --scenario test/wokwi-tests/parking_full.yaml

build-unity-tests:
	@bash -c ". $${IDF_PATH:-/opt/esp/idf}/export.sh && cd test/unity-hw-tests && idf.py build"
	@echo "Unity test firmware built in test/unity-hw-tests/build/"

test-unity-wokwi: build-unity-tests
	wokwi-cli --timeout 120000 --scenario test/wokwi-tests/unity_hw_tests.yaml

# ESP32 Coverage Note:
# gcov coverage for ESP32 requires JTAG/OpenOCD - the gcov runtime
# (__gcov_merge_add etc.) is not available for Xtensa cross-compilation.
# Use host tests (make coverage-run) for code coverage instead.
#
# The targets below are kept for documentation but will not work without JTAG:
# - build-coverage: Would build with coverage flags (requires JTAG to collect)
# - test-wokwi-coverage: Would run Wokwi and dump coverage (not possible without JTAG)

# Placeholder - ESP32 coverage requires JTAG hardware
build-coverage:
	@echo "ERROR: ESP32 gcov coverage requires JTAG/OpenOCD hardware."
	@echo "The gcov runtime is not available for Xtensa cross-compilation."
	@echo ""
	@echo "For code coverage, use host tests instead:"
	@echo "  make coverage-run"
	@echo ""
	@echo "For ESP32 coverage with JTAG, see:"
	@echo "  https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/app_trace.html#gcov-source-code-coverage"
	@exit 1

# Placeholder - requires JTAG
test-wokwi-coverage:
	@echo "ERROR: Wokwi coverage collection is not possible without JTAG."
	@echo "Use 'make coverage-run' for host-based coverage instead."
	@exit 1


format-check:
	find main components test -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -print0 | \
	xargs -0 -r clang-format --style=file --dry-run --Werror

format:
	find main components test -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -print0 | \
	xargs -0 -r clang-format --style=file -i


# clang-tidy Database erzeugen (entspricht "Lint - gen tidy db")
lint-tidy-db:
	cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	python3 tools/gen_tidy_compile_commands.py

# clang-tidy auf allen Dateien (entspricht "Lint - tidy")
lint-tidy: lint-tidy-db
	run-clang-tidy -p build/compile_commands_tidy -header-filter='^(main|components|examples|test)/' -j $(JOBS)

# clang-tidy nur auf geänderten Dateien (entspricht "Lint - tidy (changed)")
lint-tidy-changed: lint-tidy-db
	git ls-files | grep -E '\.(c|cpp|h|hpp)$' | grep -v '^build/' | grep -v '^bootloader/' | grep -v '^esp-idf/' | xargs -I{} clang-tidy -p build/compile_commands_tidy -header-filter='^(main|components|examples|test)/' {}

coverage-run:
	cmake -S test -B build-host -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
	cmake --build build-host
	ctest --test-dir build-host --output-on-failure
#     gcovr -r . --xml -o coverage.xml || true
	gcovr -r . --html --html-details -o build-host/coverage.html
	@echo "Coverage report generated: build-host/coverage.html"

# GitHub Actions lokal testen mit act
act-test:
	act --container-architecture linux/amd64
# Spezifischen Job testen
act-build:
	act --container-architecture linux/amd64 --job build
act-lint:
	act --container-architecture linux/amd64 --job lint
act-format:
	act --container-architecture linux/amd64 --job format

# CI Pipeline komplett ausführen
ci-local:
	@echo "=== Running complete CI pipeline locally ==="
	@echo "1. Format check..."
	@make format-check
	@echo "2. Lint check..."
	@make lint-check
	@echo "3. Build..."
	@make build-local
	@echo "4. Tests..."
	@make test-local
	@echo "5. Coverage..."
	@make coverage-run
	@echo "=== All checks passed ==="


###############################################
# Docker image build & push (Devcontainer/CI) #
###############################################

# Registry/Image settings
# Override via CLI: make docker-build IMAGE=ghcr.io/<user>/<image>:<tag>
REGISTRY ?= ghcr.io
OWNER    ?= 0xfischer
REPO     ?= parking_garage_control_system
IMAGE    ?= $(REGISTRY)/$(OWNER)/$(REPO)-dev:latest

# Build args (optional)
ESP_IDF_VERSION ?= v5.2.2
BUILD_ARGS ?= --build-arg ESP_IDF_VERSION=$(ESP_IDF_VERSION)

# Build context and Dockerfile (defaults to Devcontainer)
DOCKER_CONTEXT ?= .
DOCKERFILE ?= .devcontainer/Dockerfile

# Buildx / platform (optional)
PLATFORM ?= linux/amd64
USE_BUILDX ?= 1

docker-release:
	TAG=$(TAG) REGISTRY=$(REGISTRY) OWNER=$(OWNER) REPO=$(REPO) IMAGE=$(IMAGE) ESP_IDF_VERSION=$(ESP_IDF_VERSION) PLATFORM=$(PLATFORM) DOCKERFILE=$(DOCKERFILE) DOCKER_CONTEXT=$(DOCKER_CONTEXT) USE_BUILDX=$(USE_BUILDX) \
	tools/docker_release.sh

# Print important env vars (after sourcing .env if present)
env-print:
	@set -e; \
	if [ -f .env ]; then . ./.env; fi; \
	echo "GITHUB_TOKEN=$${GITHUB_TOKEN:-}"; \
	echo "WOKWI_CLI_TOKEN=$${WOKWI_CLI_TOKEN:-}"

# Create an example .env file (not committed)
env-example:
	@if [ -f .env ]; then \
		echo ".env already exists"; \
	else \
		echo "Creating .env example"; \
		printf "GITHUB_TOKEN=\nWOKWI_CLI_TOKEN=\n" > .env; \
		echo "Fill in tokens and do NOT commit .env"; \
	fi

# Convenience target: run Wokwi workflow via act using WOKWI_CLI_TOKEN
act-wokwi:
	@set -e; \
	if [ -f .env ]; then . ./.env; fi; \
	if [ -z "$$WOKWI_CLI_TOKEN" ]; then \
		echo "WOKWI_CLI_TOKEN not set. Put it in .env or pass via 'make act-wokwi WOKWI_CLI_TOKEN=...'"; \
		exit 1; \
	fi; \
	./bin/act -W .github/workflows/wokwi-tests.yml -s WOKWI_CLI_TOKEN=$$WOKWI_CLI_TOKEN
