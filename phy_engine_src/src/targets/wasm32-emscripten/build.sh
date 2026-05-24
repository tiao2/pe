#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
ENGINE_DIR="$ROOT_DIR"
OUT_DIR="${OUT_DIR:-/wasm32-emscripten-pe-release}"

mkdir -p "$OUT_DIR"
export EM_CACHE="${EM_CACHE:-$ROOT_DIR/.cache/emscripten}"
mkdir -p "$EM_CACHE"

em++ \
  -std=c++23 \
  -O3 \
  -I"$ENGINE_DIR/include" \
  "$ENGINE_DIR/src/dll_main.cpp" \
  -fno-rtti \
  -fexceptions \
  -s DISABLE_EXCEPTION_CATCHING=0 \
  -s MODULARIZE=1 \
  -s EXPORT_ES6=1 \
  -s ENVIRONMENT=web \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s FILESYSTEM=0 \
  -s EXPORTED_RUNTIME_METHODS='["cwrap"]' \
  -s EXPORTED_FUNCTIONS='["_malloc","_free","_create_circuit","_create_circuit_ex","_destroy_circuit","_circuit_set_analyze_type","_circuit_set_tr","_circuit_set_ac_omega","_circuit_analyze","_circuit_digital_clk","_circuit_sample_u8","_circuit_set_model_digital"]' \
  -o "$OUT_DIR/phy_engine.js"

echo "Built:"
ls -la "$OUT_DIR/phy_engine.js" "$OUT_DIR/phy_engine.wasm"