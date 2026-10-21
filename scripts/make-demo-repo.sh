#!/usr/bin/env bash
# Create a fresh git repo that plays "the customer's firmware repo": a scripted, deterministic
# history on main (benign commits, optionally one planted-bug commit), plus a branch bug/<name>
# for every planted bug (good main tip + that one bug commit) for the live push demo.
#
# usage: make-demo-repo.sh <out-dir> [--bug <name> --at <k>] [--total <n>] [--subdir <dir>]
#   --bug   hardfault | baud | watchdog | crc    (omit for an all-benign main)
#   --at    position of the bug commit; commit 1 is the initial import (2 <= k <= n)
#   --total commits on main, including the import and the bug (default 12, max 13)
#   --subdir put the firmware under <dir>/ in the repo instead of the repo root
# Hashes are reproducible: author "Demo Dev", fixed dates, no hooks, no signing.
set -euo pipefail

here=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd) # firmware/
out=""; bug=""; at=""; total=12; subdir=""
while [ $# -gt 0 ]; do
    case $1 in
        --bug) bug=${2:?}; shift 2 ;;
        --at) at=${2:?}; shift 2 ;;
        --total) total=${2:?}; shift 2 ;;
        --subdir) subdir=${2:?}; shift 2 ;;
        -h|--help) sed -n '2,11p' "$0"; exit 0 ;;
        -*) echo "unknown option $1" >&2; exit 2 ;;
        *) out=$1; shift ;;
    esac
done
[ -n "$out" ] || { echo "usage: $0 <out-dir> [--bug <name> --at <k>] [--total <n>]" >&2; exit 2; }
bugs="hardfault baud watchdog crc"
if [ -n "$bug" ]; then
    case " $bugs " in *" $bug "*) ;; *) echo "unknown bug '$bug' (one of: $bugs)" >&2; exit 2 ;; esac
    [ -n "$at" ] || { echo "--bug needs --at <k>" >&2; exit 2; }
fi
[ "$total" -ge 3 ] && [ "$total" -le 13 ] || { echo "--total must be 3..13" >&2; exit 2; }
if [ -n "$bug" ]; then
    [ "$at" -ge 2 ] && [ "$at" -le "$total" ] || { echo "--at must be 2..$total" >&2; exit 2; }
fi
[ ! -e "$out" ] || [ -z "$(ls -A "$out" 2>/dev/null)" ] || { echo "$out exists and is not empty" >&2; exit 2; }
mkdir -p "$out"; out=$(cd "$out" && pwd)
fw=$out${subdir:+/$subdir}
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT

export GIT_AUTHOR_NAME="Demo Dev" GIT_AUTHOR_EMAIL="demo-dev@example.com"
export GIT_COMMITTER_NAME="Demo Dev" GIT_COMMITTER_EMAIL="demo-dev@example.com"
export GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null
g() { git -C "$out" -c core.hooksPath=/dev/null -c commit.gpgsign=false "$@"; }

base_epoch=$(date -u -d 2026-10-01T16:00:00Z +%s) # 09:00 PDT
commit() { # <index> <message>   index sets the fixed date, 6 h apart
    local d; d=$(date -u -d "@$((base_epoch + $1 * 21600))" +%Y-%m-%dT%H:%M:%SZ)
    g add -A
    GIT_AUTHOR_DATE=$d GIT_COMMITTER_DATE=$d g commit -q --no-verify -m "$2"
}

# --- benign edits: small, in files the bug patches do not touch near their hunks ---
sedi() { sed -i "$@"; }
benign() { # <n> ; sets MSG
    cd "$fw"
    case $1 in
    1) cat > README.md <<'R'
# Demo firmware

Bare-metal self-test firmware for the NUCLEO-F446RE.

    make            # build/fw.elf, build/fw.bin
R
       MSG="docs: add README with build instructions" ;;
    2) sedi 's|^void uart_putc(char c)|/* Polled TX: blocks until the data register is empty. */\nvoid uart_putc(char c)|' src/uart.c
       MSG="uart: document that TX is polled" ;;
    3) sedi 's|APP_VERSION "1.0.0"|APP_VERSION "1.0.1"|' src/app.h
       MSG="Bump version to 1.0.1" ;;
    4) sedi '1a /* Bitwise on purpose: no 1 KiB lookup table in flash. */' src/crc32.c
       MSG="crc32: note why it is bitwise" ;;
    5) python3 - <<'P'
p='src/selftest.c'
s=open(p).read()
s=s.replace('struct test { const char *name;','''static void test_bitops(void)
{
    uint32_t v = 0x80000001u;
    int ok = (v >> 31) == 1u && (v & 0xFFu) == 1u && (uint32_t)(0u - 1u) == 0xFFFFFFFFu;
    report("bitops", ok, "shift or mask wrong");
}

struct test { const char *name;''',1)
s=s.replace('    { "cmd_dispatch", test_cmd },\n','    { "cmd_dispatch", test_cmd },\n    { "bitops", test_bitops },\n',1)
open(p,'w').write(s)
P
       MSG="selftest: add a bitops test" ;;
    6) sedi 's|^int rb_push(|/* Returns -1 when full; the byte is dropped. */\nint rb_push(|' src/ringbuf.c
       MSG="ringbuf: document the full case" ;;
    7) sedi 's|^FORCE:|size: build/fw.elf\n\t$(SIZE) $<\n\nFORCE:|' Makefile
       MSG="make: add a size target" ;;
    8) sedi 's|APP_VERSION "1.0.1"|APP_VERSION "1.1.0"|' src/app.h
       MSG="Bump version to 1.1.0" ;;
    9) sedi 's|^void systick_init(void)|/* 1 kHz tick; millis() wraps after 49 days. */\nvoid systick_init(void)|' src/timing.c
       MSG="timing: comment the tick rate" ;;
    10) sedi 's|^void wdt_kick(void)|/* Any write of 0xAAAA reloads the counter. */\nvoid wdt_kick(void)|' src/wdt.c
       MSG="wdt: explain the reload key" ;;
    11) printf '\n## Output\n\nThe board prints `LW ...` lines on USART2 at 115200 8N1.\n' >> README.md
       MSG="docs: describe the serial output" ;;
    12) sedi 's|read before anything can disturb it|read first, before any peripheral init|' src/main.c
       MSG="main: tidy a comment" ;;
    *) echo "out of benign edits" >&2; exit 2 ;;
    esac
    cd "$out"
}

bug_commit() { # <name> <index>
    git mailinfo "$tmp/msg" "$tmp/patch" < "$here/bugs/$1.patch" > "$tmp/hdr"
    local subj; subj=$(sed -n 's/^Subject: //p' "$tmp/hdr")
    ( cd "$out" && git apply ${subdir:+--directory="$subdir"} "$tmp/patch" )
    commit "$2" "$subj

$(cat "$tmp/msg")"
}

import_tree() {
    mkdir -p "$fw"
    cp -r "$here/Makefile" "$here/link.ld" "$here/.gitignore" "$here/src" "$fw/"
}

# chain <bug-or-empty> <at> <n-commits>: builds on the current (fresh) branch
chain() {
    local b=$1 k=$2 n=$3 pos nb=0
    import_tree; commit 1 "Initial import of the demo firmware"
    for ((pos = 2; pos <= n; pos++)); do
        if [ -n "$b" ] && [ "$pos" -eq "$k" ]; then
            bug_commit "$b" "$pos"
        else
            nb=$((nb + 1)); benign "$nb"; commit "$pos" "$MSG"
        fi
    done
}

if [ -n "$bug" ]; then
    # Good history first (n = total commits), so bug/<name> branches hang off a clean tip.
    g init -q -b tmp-good
    chain "" "" "$total"
    for b in $bugs; do
        g checkout -q -b "bug/$b" tmp-good
        bug_commit "$b" $((total + 1))
    done
    g checkout -q --orphan main
    g rm -rfq . ; git -C "$out" clean -fdxq
    chain "$bug" "$at" "$total"
    g branch -q -D tmp-good
else
    g init -q -b main
    chain "" "" "$total"
    for b in $bugs; do
        g checkout -q -b "bug/$b" main
        bug_commit "$b" $((total + 1))
    done
    g checkout -q main
fi
echo "demo repo: $out"
g log --format='%h %ad %s' --date=short main
echo "branches: $(g branch --format='%(refname:short)' | tr '\n' ' ')"
