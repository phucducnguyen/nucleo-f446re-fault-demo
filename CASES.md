# Test cases and answer key

Twelve code cases and four hardware cases. The `case/*` branches have commit messages that don't
name the bug, so they can be used to check a diagnosis without giving the answer away: don't show
this file to whatever is being checked.

Every code branch is `main` plus one commit. All behaviour below was seen on a NUCLEO-F446RE
(Oct 20, 2026); register values and addresses depend on the build.

## Code cases

| Branch | Commit message | What the board shows | Cause |
| --- | --- | --- | --- |
| `bug/hardfault` | cmd: add 'status' command table entry | Three tests pass, then `LW HARDFAULT` with `cfsr=0x00020000` (INVSTATE), pc=0, LR in `cmd_dispatch` | NULL handler in the command table, called without a check |
| `bug/baud` | clock: run the core at 84 MHz from the PLL | Garbage at 115200; clean lines at about 302,158 baud | UART divisor still computed for a 16 MHz bus clock; APB1 now runs at 42 MHz |
| `bug/watchdog` | sensor: add calibration at startup | `LW BOOT ... reset=IWDG,PIN` about once a second, no tests | 1.5 s busy-wait without a watchdog kick |
| `bug/crc` | crc32: do not hash the trailing NUL | `LW TEST crc32 FAIL got=0x9ae0daaf want=0xcbf43926`, `LW RESULT FAIL` | Off-by-one loop bound (`i + 1 < len`) |
| `case/01` | selftest: keep a per-test timing trace in SRAM | `LW HARDFAULT` before any test, `cfsr=0x00008200` (PRECISERR, BFARVALID), `bfar=0x20020000`, PC in `trace_stamp` | Trace buffer based one past the end of SRAM |
| `case/02` | selftest: log which commands fail the smoke test | Three tests pass, then `cfsr=0x02000000` (DIVBYZERO), PC in `test_cmd` | Divide by a failure count that is zero; the same commit enables the divide trap |
| `case/03` | ringbuf: add rb_peek32 for the framing header | crc32 passes, then `cfsr=0x01000000` (UNALIGNED), PC in `rb_peek32` | 32-bit load from an odd address; the same commit enables the unaligned trap |
| `case/04` | ringbuf: add an order-sensitive digest | crc32 passes, then a burst of truncated `LW HARDFAULT` text with no registers, then `reset=IWDG,PIN` on every boot | Unbounded recursion on an empty queue: the stack overflows, so the fault handler can't finish its dump, and the watchdog resets the board |
| `case/05` | ringbuf: keep one slot free | `LW TEST ringbuf FAIL fifo order or wrap wrong`, `LW RESULT FAIL` | The buffer refuses a push one slot early |
| `case/06` | uart: flush the banner before the self-tests start | `LW BOOT` lines with `reset=IWDG,PIN` about once a second, no tests | Waits on a UART flag that never sets, without kicking the watchdog |
| `case/07` | uart: name the alternate-function number | No output at all; the LED behaves as if the firmware runs | Wrong alternate-function number on the UART pins |
| `case/08` | wdt: name the IWDG key values | Same as `main`: all tests pass | Nothing: a harmless change, the control case |

Notes:
- `case/04` is a trap for parsers: a line that starts with `LW HARDFAULT` but carries no registers.
  Emulators show it differently (QEMU locks up with no dump), so trust the board.
- `case/06` and `bug/watchdog` look alike on the wire. Telling them apart needs the diff.
- `case/07` looks like a dead board or a bad cable. The real evidence is that the flash succeeded
  and the same board prints with `main`.

## Hardware cases (no code change; run `main`)

Confirmed on the board (Oct 20): the jumper pull and reinsert (each change shows on the next
health line, about a second later), the RESET button and the pulled USB cable. Not checked: the
short to GND.

| Action | What the board shows |
| --- | --- |
| Pull the A0-A1 jumper while running | `LW HEALTH <n> loopback=OPEN` from the next health line |
| Wire A1 to GND | `loopback=STUCK` |
| Press the black RESET button | A new `LW BOOT ... reset=PIN` without a flash |
| Pull the USB cable | The serial port disappears. On replug the board has already booted before the port can be reopened, so the `LW BOOT` line is usually missed; the `LW ALIVE`/`LW HEALTH` counter restarting from a low number shows the reboot |

With the same firmware and no new commit, any of these points to the hardware, not the code.
