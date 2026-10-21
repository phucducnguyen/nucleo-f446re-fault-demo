#!/usr/bin/env bash
# Run a firmware ELF in QEMU (netduinoplus2, STM32F405) and print USART2 to stdout.
# usage: qemu-run.sh <fw.elf> <seconds>
# QEMU_STDIN=1 forwards stdin to USART2 RX (type `ping`, `test loopback`).
# Uses a local qemu-system-arm, else the f446-fw-tools Docker image (docker/Dockerfile).
set -euo pipefail
elf=${1:?usage: qemu-run.sh <fw.elf> <seconds>}
secs=${2:-5}
[ -f "$elf" ] || { echo "no such file: $elf" >&2; exit 2; }
# -serial maps to USART1..USART6 in order, so USART2 is the second -serial.
args=(-M netduinoplus2 -display none -monitor none -serial null -serial stdio -kernel)
if command -v qemu-system-arm >/dev/null 2>&1; then
    timeout "$secs" qemu-system-arm "${args[@]}" "$elf" || true
else
    dir=$(cd "$(dirname "$elf")" && pwd)
    timeout "$((secs + 20))" docker run --rm ${QEMU_STDIN:+-i} -v "$dir:/e:ro" f446-fw-tools \
        timeout "$secs" qemu-system-arm "${args[@]}" "/e/$(basename "$elf")" || true
fi
