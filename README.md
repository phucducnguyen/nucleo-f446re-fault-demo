# NUCLEO-F446RE fault demo firmware

Small bare-metal test firmware for the ST NUCLEO-F446RE, with planted bugs. It is test data: the
board a monitoring agent watches in a demo. It has no vendor library, only its own startup code,
linker script and Makefile.

On reset it runs five self-tests and reports the results over the ST-LINK virtual COM port. Then it
prints a health line every second and answers a few line commands. Each planted bug breaks it in a
different way that can be diagnosed from the serial output.

## Wiring

- A USB cable to the ST-LINK port (Mini-B).
- One female-female jumper wire between **A0 and A1**. These are the first two pins of the Arduino
  analog header CN8 (PA0 and PA1). Go by the silkscreen labels.

Serial: USART2 on PA2/PA3, which the ST-LINK exposes as a virtual COM port. 115200 8N1.
LD2 (PA5) blinks at 1 Hz after a pass, and at 5 Hz after a failure or a fault.

## Build

You need the Arm GNU bare-metal toolchain (`arm-none-eabi-`). It builds the same way on x86-64 and
aarch64 Linux.

    sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi make
    make                                        # build/fw.elf, build/fw.bin, build/fw.map
    make TOOLCHAIN=/opt/arm/bin/arm-none-eabi-  # toolchain from a tarball
    make FW_VERSION=abc1234                     # default: git short sha, else "dev"

Without a local toolchain:

    docker build -t f446-fw-tools docker
    docker run --rm -v "$PWD":/w -w /w -u "$(id -u):$(id -g)" f446-fw-tools make FW_VERSION=dev

## Flash

Copy `build/fw.bin` onto the USB drive that the ST-LINK exposes (labelled `NODE_F446RE`; some
hosts show it truncated as `NOD_F446RE`). The board resets
itself when the copy finishes. The alternatives:

    STM32_Programmer_CLI -c port=SWD -w build/fw.bin 0x08000000 -v -rst
    openocd -f board/st_nucleo_f4.cfg -c "program build/fw.elf verify reset exit"

Watch the output with `picocom -b 115200 /dev/ttyACM0`.

## Serial output

Every line meant for a machine starts with `LW `. Any other line is free text.

    LW BOOT fw=<sha> reset=<CAUSE>[,<CAUSE>...] csr=0x<RCC_CSR>
    LW TEST <name> PASS
    LW TEST <name> FAIL <detail>
    LW RESULT PASS | LW RESULT FAIL
    LW HARDFAULT pc=0x.. lr=0x.. psr=0x.. cfsr=0x.. hfsr=0x.. bfar=0x.. mmfar=0x.. sp=0x..
    LW ALIVE <n>
    LW HEALTH <n> loopback=OK|OPEN|STUCK
    LW REPLY <command> <text>

- `CAUSE` is decoded from RCC_CSR (`LPWR WWDG IWDG SFT POR PIN BOR`). The flags are cleared after
  they are printed. A press of the RESET button gives `reset=PIN`.
- A run ends with exactly one `LW RESULT` or one `LW HARDFAULT` line.
- After `LW RESULT`, the board prints `LW ALIVE <n>` once a second, each followed by a `LW HEALTH`
  line. After a HardFault, it repeats the `LW HARDFAULT` line once a second instead.

A good build, with the jumper in place:

    LW BOOT fw=<sha> reset=POR,PIN,BOR csr=0x0e000000
    LW TEST crc32 PASS
    LW TEST ringbuf PASS
    LW TEST systick PASS
    LW TEST cmd_dispatch PASS
    LW TEST loopback PASS
    LW RESULT PASS
    LW ALIVE 1
    LW HEALTH 1 loopback=OK

## Live health

PA0 is a push-pull output and PA1 an input with a pull-up. The loopback check drives PA0 low and
then high, and reads PA1 each time:

- PA1 follows both times: `OK`.
- PA1 stays high: `OPEN` (the jumper is pulled).
- PA1 stays low: `STUCK` (shorted to ground).

The check is the fifth boot self-test, and it runs again after every `LW ALIVE` line. The firmware
only reports the state. Deciding what counts as an incident is up to whatever watches it.

## Commands

Send commands on USART2 RX: one line, ended by `\r` or `\n`, at most 32 characters.

    ping           ->  LW REPLY ping ok
    test <name>    ->  LW TEST <name> PASS|FAIL ...  then  LW REPLY test done
                       (names: crc32 ringbuf systick cmd_dispatch loopback)
    anything else  ->  LW REPLY <first word> unknown

The board drops longer lines and lines hit by a receiver overrun. It echoes nothing.

## Planted bugs

Each branch `bug/<name>` is the good firmware plus one commit with a believable message. The same
changes are in `bugs/*.patch`.

| Branch | Commit | What the board does |
| --- | --- | --- |
| `bug/hardfault` | "cmd: add 'status' command table entry" | The new entry's handler is NULL and `cmd_dispatch` calls it without a check. Three tests pass, then `LW HARDFAULT ... cfsr=0x00020000` (INVSTATE) repeats once a second. The stacked LR points just after the call in `cmd_dispatch` (`src/cmd.c`). |
| `bug/baud` | "clock: run the core at 84 MHz from the PLL" | APB1 now runs at 42 MHz, but the UART divisor is still computed for 16 MHz. The real baud rate is about 302,158, so a terminal at 115200 shows garbage. |
| `bug/watchdog` | "sensor: add calibration at startup" | A 1.5 s busy-wait without a watchdog kick. Every boot prints `reset=IWDG,PIN` (the watchdog reset also pulls the reset pin) and never reaches the tests. |
| `bug/crc` | "crc32: do not hash the trailing NUL" | `LW TEST crc32 FAIL got=0x9ae0daaf want=0xcbf43926`, then `LW RESULT FAIL`. |

To bring a case onto `main` as a fresh commit (the way a teammate's push would arrive), use
`git cherry-pick bug/hardfault` (or any `case/NN`); `git show HEAD` then shows only that change.

Eight more cases are on `case/01` to `case/08`, with messages that don't name the bug. `CASES.md`
is the answer key for all twelve plus the hardware cases.

`scripts/make-demo-repo.sh <dir>` builds a longer scripted history for bisect practice: ordinary
commits on `main`, with one bug commit at a chosen position (`--bug crc --at 7`).

## Emulator

`scripts/qemu-run.sh build/fw.elf 4` runs the firmware in QEMU (`netduinoplus2`, an STM32F405).
QEMU has no GPIO, clock or watchdog model, so build with `make SIM=1` there: the loopback check then
reports `OK` without touching the pins. The baud and watchdog bugs only show on a real board.

## License

MIT, see `LICENSE`.
