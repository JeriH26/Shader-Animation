#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_FILE="$ROOT_DIR/scripts/ray_sphere_learning.cpp"
OUT_FILE="$ROOT_DIR/build/ray_sphere_learning"

mkdir -p "$ROOT_DIR/build"

if command -v clang++ >/dev/null 2>&1; then
  CXX=clang++
elif command -v g++ >/dev/null 2>&1; then
  CXX=g++
else
  echo "No C++ compiler found (clang++/g++)."
  exit 1
fi

"$CXX" -std=c++17 -O2 -Wall -Wextra "$SRC_FILE" -o "$OUT_FILE"

"$OUT_FILE"
