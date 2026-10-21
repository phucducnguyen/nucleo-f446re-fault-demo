# Recorded serial output

Raw bytes from the ST-LINK virtual COM port at 115200 baud, about 6 to 8 seconds after flashing
each build on a NUCLEO-F446RE with the A0-A1 jumper in (Oct 20, 2026). Use them to test a serial
reader without a board.

- `main.raw`, `hardfault.raw`, `baud.raw`, `watchdog.raw`, `crc.raw`: `main` and the `bug/*` branches.
- `case01.raw` to `case08.raw`: the `case/*` branches.
- `baud.raw` is unreadable at 115200 on purpose (the board sends at about 302,158 baud).
- `case07.raw` is empty on purpose (no output at all).
- `case04.raw` holds the truncated `LW HARDFAULT` burst and the watchdog reboots.

The `fw=` values are the commit ids of the builds that were recorded; the source is the same as
on the branches.
