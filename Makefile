-include .env
export

.PHONY: test-local build-local build-ci format-check lint-check coverage-run act-test fullclean lint-tidy-db lint-tidy lint-tidy-changed wokwi-test wokwi-test-ci \
	env-print env-example act-wokwi docker-release init test-wokwi-coverage build-coverage build-unity-tests test-unity-wokwi \
	docs docs-site docs-deploy docs-serve docs-clean docs-reset

JOBS := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)

init:
	@if [ -z "$$IDF_PATH" ]; then \
		echo "Error: IDF_PATH not set. Please set it to your ESP-IDF installation path."; \
		echo "Example: export IDF_PATH=~/esp/esp-idf"; \
		exit 1; \
	fi; \
	echo "Sourcing ESP-IDF environment from $$IDF_PATH/export.sh..."; \
	@bash -c "export IDF_PATH_FORCE=1 && . $$IDF_PATH/export.sh && echo \"ESP-IDF environment initialized successfully\""

fullclean:
	@echo "=== Full clean ==="
	@bash -c 'idf.py fullclean' || true
	@rm -rf build sdkconfig 2>/dev/null || true
	@bash -c "cd test/unity-hw-tests && idf.py fullclean" || true
	@rm -rf test/unity-hw-tests/build test/unity-hw-tests/sdkconfig 2>/dev/null || true
	@rm -f build-host || true
	@echo "✓ Clean complete"

build-local:
	@bash -c 'idf.py build'

build-ci:
	@rm -f sdkconfig
	@bash -c 'idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.ci" build'

test-host:
	cmake -S test -B build-host -DCMAKE_BUILD_TYPE=Debug
	cmake --build build-host
	ctest --test-dir build-host --output-on-failure
test-wokwi:
	wokwi-cli --scenario test/wokwi-tests/parking_full.yaml
test-wokwi-full: build-ci
	@failed_tests=""
	@for scenario in test/wokwi-tests/console_full.yaml test/wokwi-tests/entry_exit_flow.yaml test/wokwi-tests/parking_full.yaml; do \
		echo "Running Wokwi test: $$scenario"; \
		if ! wokwi-cli --scenario $$scenario; then \
			echo "Test failed: $$scenario"; \
			failed_tests="$$failed_tests $$scenario"; \
		fi; \
	done; \
	if [ -n "$$failed_tests" ]; then \
		echo "The following tests failed:"; \
		for test in $$failed_tests; do \
			echo " - $$test"; \
		done; \
		exit 1; \
	else \
		echo "All Wokwi tests passed successfully."; \
	fi

test-unity-hw-tests-wokwi:
	@bash -c "cd test/unity-hw-tests && idf.py build"
	@bash -c "cd test/unity-hw-tests && wokwi-cli --scenario ../wokwi-tests/unity_hw_tests.yaml"
build-unity-tests:
	@bash -c "cd test/unity-hw-tests && idf.py build"
	@echo "Unity test firmware built in test/unity-hw-tests/build/"

test-unity-wokwi: build-unity-tests
	cd test/unity-hw-tests && wokwi-cli --timeout 60000 --scenario ../wokwi-tests/unity_hw_tests.yaml

test-unity-qemu:
	@bash -c "cd test/unity-hw-tests && idf.py build"
	@bash -c "cd test/unity-hw-tests && idf.py qemu monitor"

test-unity-qemu-cov:
	@bash -c "cd test/unity-hw-tests && rm -rf build sdkconfig"
	@bash -c "cd test/unity-hw-tests && idf.py -D SDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.coverage' build"
	@echo "Generating QEMU flash image..."
	@cd test/unity-hw-tests/build && esptool.py --chip=esp32 merge_bin --output=qemu_flash.bin --fill-flash-size=4MB --flash_mode dio --flash_freq 40m --flash_size 4MB 0x1000 bootloader/bootloader.bin 0x10000 unity_hw_tests.bin 0x8000 partition_table/partition-table.bin
	@echo "Running QEMU with coverage..."
	@cd test/unity-hw-tests && qemu-system-xtensa -nographic -M esp32 -m 4M \
		-drive file=build/qemu_flash.bin,if=mtd,format=raw \
		-global driver=timer.esp32.timg,property=wdt_disable,value=true \
		-serial mon:stdio \
		-semihosting \
		-semihosting-config enable=on,target=native \
		| tee test_output.log
	@echo "Coverage run complete. Generating report..."
	@mkdir -p test/unity-hw-tests/build/coverage_report
	@cd test/unity-hw-tests && gcovr --root . --html --html-details -o build/coverage_report/index.html
	@echo "Coverage report generated at test/unity-hw-tests/build/coverage_report/index.html"


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

format-changed:
	git ls-files | grep -E '\.(c|cpp|h|hpp)$$' | xargs -r -I{} clang-format --style=file -i {}


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


###############################################
# Documentation (Doxygen + Coverage)          #
###############################################

# Build Doxygen documentation
docs:
	doxygen Doxyfile
	@echo "Documentation generated: html/index.html"

# Prepare complete gh-pages site locally (Doxygen + Coverage + README)
docs-site: docs coverage-run
	mkdir -p docs/coverage
	# Publish Doxygen HTML at site root
	cp -r html/* docs/
	# Coverage report (HTML + CSS)
	cp build-host/coverage.html docs/coverage/index.html || true
	cp build-host/coverage.*.html docs/coverage/ 2>/dev/null || true
	cp build-host/coverage.css docs/coverage/ 2>/dev/null || true
	# README for reference (Doxygen main page already renders README)
	cp README.md docs/ || true
	# Optional media
	[ -f demo.gif ] && cp demo.gif docs/ || true
	# Ensure GitHub Pages serves static files as-is
	touch docs/.nojekyll
	@echo "Site prepared in docs/"
	@echo "  - Doxygen root: docs/index.html"
	@echo "  - Coverage:     docs/coverage/index.html"

# Serve the docs/ folder locally with Jekyll (requires bundle install in docs/)
# cd docs
# bundle config set --local path 'vendor/bundle'
# bundle install
docs-serve-install:
	@cd docs && \
		bundle config set --local path 'vendor/bundle' && \
		bundle install

docs-serve: docs-serve-install
	@# Ensure README.md is present in docs/ for include_relative in index.md
	@[ -f docs/README.md ] || cp README.md docs/
	@cd docs && bundle exec jekyll serve --safe -H 0.0.0.0

# Clean generated documentation and site artifacts
docs-clean:
	@echo "=== Cleaning generated documentation and site artifacts ==="
	rm -rf html
	rm -rf docs/_site docs/.jekyll-cache docs/.sass-cache docs/.nojekyll docs/coverage
	@cd docs && find . -mindepth 1 \
	 ! -path './_config.yml' \
	 ! -path './_layouts' ! -path './_layouts/*' \
	 ! -path './index.md' \
	 ! -path './footer.html' \
	 ! -path './mermaid-init.js' \
	 ! -path './mermaid.md' \
	 ! -path './Gemfile' \
	 ! -path './Gemfile.lock' \
	 ! -path './vendor' ! -path './vendor/*' \
	 ! -path './demo.gif' \
	 -exec rm -rf {} +
	@echo "✓ Docs cleaned."

# Hard reset: also remove Bundler vendor cache (forces bundle install next time)
docs-reset: docs-clean
	rm -rf docs/vendor
	@echo "✓ Docs vendor removed (fresh bundle install required)."

# Deploy to gh-pages branch (requires push permissions)
docs-deploy: docs-site
	@command -v ghp-import >/dev/null 2>&1 || pipx install ghp-import --quiet
	ghp-import -n -p -f docs
	@echo "Deployed to gh-pages branch"

docs-serve-full: docs-clean docs-site docs-serve
	@echo "Full documentation build and deployment complete."
