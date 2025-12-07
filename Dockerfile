# Minimal tooling image for formatting and optional linting
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    clang-format \
    clang-tidy \
    cmake \
    git \
    python3 \
    ca-certificates \
    build-essential \
 && rm -rf /var/lib/apt/lists/*

# Default: do not enforce clang-tidy in hooks inside container unless explicitly enabled
ENV ENABLE_TIDY=0

WORKDIR /workspace

# Usage:
#   docker build -t pgcs-tools .
#   docker run --rm -it -v "$PWD":/workspace pgcs-tools bash
# Inside container:
#   cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
#   python3 tools/gen_tidy_compile_commands.py
#   run-clang-tidy -p build/compile_commands_tidy -header-filter='^(main|components|examples|test)/' -j $(nproc)
