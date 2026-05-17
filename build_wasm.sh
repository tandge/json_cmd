#!/bin/bash
# build_wasm.sh - Build WASM binary and JS glue for the web UI
set -e

PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
OUTPUT_DIR="${PROJECT_ROOT}/docs"

# Source emsdk if not already in PATH
if ! command -v emcc &>/dev/null; then
    source "/home/xixi/emsdk/emsdk_env.sh" 2>/dev/null || true
fi

echo "Building WASM..."
cd "${PROJECT_ROOT}"

emcc main.cpp \
    -I"${PROJECT_ROOT}" \
    --bind \
    -s MODULARIZE=1 \
    -s EXPORT_NAME='JsonCmd' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s WASM=1 \
    -s SINGLE_FILE=1 \
    -s DISABLE_EXCEPTION_CATCHING=0 \
    -O2 \
    -o "${OUTPUT_DIR}/json_cmd.js"

echo "WASM build complete (SINGLE_FILE mode, wasm embedded in js):"
echo "  ${OUTPUT_DIR}/json_cmd.js"
