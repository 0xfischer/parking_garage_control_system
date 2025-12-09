-include .env
export

.PHONY: test-local build-local format-check lint-check coverage-run act-test fullclean lint-tidy-db lint-tidy lint-tidy-changed wokwi-test wokwi-test-ci \
	env-print env-example act-wokwi docker-release init

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
	idf.py fullclean || true
	rm -rf build sdkconfig || true

build-local:
	idf.py build

test-local:
	cmake -S test -B build-host -DCMAKE_BUILD_TYPE=Debug
	cmake --build build-host
	ctest --test-dir build-host --output-on-failure

format-check:
	find main components test -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -print0 | \
	xargs -0 -r clang-format --style=file --dry-run --Werror

format-apply:
	find main components test -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -print0 | \
	xargs -0 -r clang-format --style=file -i

# separater Cppcheck-Run (robust mit Null-Separator)

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

# Coverage
coverage-run:
	cmake -S test -B build-host -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
	cmake --build build-host
	ctest --test-dir build-host --output-on-failure
#     gcovr -r . --xml -o coverage.xml || true
	gcovr -r . --html --html-details -o build-host/coverage.html
	@echo "Coverage report generated: build-host/coverage.html"

# GitHub Actions lokal testen mit act
act-test:
	./bin/act --container-architecture linux/amd64

# Spezifischen Job testen
act-build:
	./bin/act --container-architecture linux/amd64 --job build
act-lint:
	./bin/act --container-architecture linux/amd64 --job lint

act-format:
	./bin/act --container-architecture linux/amd64 --job format

wokwi-test:
	wokwi-cli --scenario test/wokwi/console_full.yaml
	# wokwi-cli --scenario test/wokwi/entry_exit_flow.yaml
	# wokwi-cli --scenario test/wokwi/full_capacity.yaml

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