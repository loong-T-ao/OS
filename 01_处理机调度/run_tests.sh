#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="$SCRIPT_DIR/output"
TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
OUTPUT_FILE="$OUTPUT_DIR/scheduler_result_${TIMESTAMP}.txt"

mkdir -p "$OUTPUT_DIR"

echo "[1/3] 清理旧构建..."
make -C "$SCRIPT_DIR" clean >/dev/null

echo "[2/3] 编译调度程序..."
make -C "$SCRIPT_DIR"

echo "[3/3] 运行全部算法并保存结果..."
"$SCRIPT_DIR/bin/scheduler" -i "$SCRIPT_DIR/tests/sample_processes.txt" -a all -q 2 | tee "$OUTPUT_FILE"

echo
echo "结果已保存到: $OUTPUT_FILE"
