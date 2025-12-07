# Contributing

## Setup
- Linux + zsh
- ESP-IDF v5.2.x installed or use ESP-IDF Action/Docker
- CMake, Python3, clang-format (optional clang-tidy)

## Workflow
- Branches: `feat/*`, `fix/*`, `chore/*`
- Conventional Commits recommended
- Open PRs with template, ensure CI checks pass

## Running CI locally
- Format:
  ```zsh
  find . -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -not -path "./build/*" -not -path "./bootloader/*" -not -path "./esp-idf/*" -print0 | xargs -0 clang-format -i
  ```
- Build (ESP-IDF):
  ```zsh
  idf.py build
  ```
- Docs:
  ```zsh
  doxygen Doxyfile
  ```
- Coverage (host-only):
  ```zsh
  cmake -S test -B build-host -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
  cmake --build build-host
  ctest --test-dir build-host --output-on-failure
  gcovr -r . --html --html-details -o coverage.html
  ```

## Code Style
- `.clang-format` defines style (LLVM-based with overrides)
- Pre-commit hook checks formatting

## Tidy
- Optional; disabled by default
- Enable on push with `ENABLE_TIDY=1`

