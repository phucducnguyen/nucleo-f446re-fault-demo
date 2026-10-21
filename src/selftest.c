#include "app.h"

static int fails;

/* Per-test timings, kept at the top of SRAM so they sit clear of .bss and the stack. */
#define TRACE_BASE ((volatile uint32_t *)(0x20000000u + 128u * 1024u))
static uint32_t trace_n;

static void trace_stamp(void)
{
    uint32_t now = millis();
    uint32_t prev = TRACE_BASE[trace_n ? trace_n - 1u : 0u];
    TRACE_BASE[trace_n++ & 15u] = now - prev;
}

static void report(const char *name, int ok, const char *detail)
{
    trace_stamp();
    uart_puts("LW TEST ");
    uart_puts(name);
    if (ok) {
        uart_puts(" PASS\n");
    } else {
        uart_puts(" FAIL ");
        uart_puts(detail);
        uart_puts("\n");
        fails++;
    }
    wdt_kick();
}

static void test_crc32(void)
{
    uint32_t got = crc32("123456789", 9);
    if (got == 0xCBF43926u) {
        report("crc32", 1, "");
        return;
    }
    uart_puts("LW TEST crc32 FAIL got=");
    uart_hex32(got);
    uart_puts(" want=0xcbf43926\n");
    fails++;
    wdt_kick();
}

static void test_ringbuf(void)
{
    struct ringbuf rb;
    uint8_t b, accepted = 0, ok = 1;
    rb_init(&rb);
    for (uint8_t i = 0; i < 12; i++) accepted += rb_push(&rb, i) == 0;
    if (accepted != RB_SIZE) ok = 0;
    for (uint8_t i = 0; i < 3; i++) if (rb_pop(&rb, &b) || b != i) ok = 0;
    for (uint8_t i = 100; i < 103; i++) if (rb_push(&rb, i)) ok = 0; /* wraps */
    for (uint8_t i = 3; i < RB_SIZE; i++) if (rb_pop(&rb, &b) || b != i) ok = 0;
    for (uint8_t i = 100; i < 103; i++) if (rb_pop(&rb, &b) || b != i) ok = 0;
    if (rb_pop(&rb, &b) == 0) ok = 0;
    report("ringbuf", ok, "fifo order or wrap wrong");
}

static void test_systick(void)
{
    uint32_t t0 = millis();
    uint32_t n = 0;
    while (millis() - t0 < 3u && n < 400000u) { n++; wdt_kick(); }
    report("systick", millis() - t0 >= 3u, "millis() did not advance");
}

static void test_cmd(void)
{
    int ok = cmd_dispatch("ping", "") == 0 && cmd_dispatch("echo", "abc") == 3
             && cmd_dispatch("nope", "") == -1;
    /* Smoke every registered command once. */
    for (size_t i = 0; i < cmd_count(); i++)
        if (cmd_dispatch(cmd_name(i), "") < 0) ok = 0;
    report("cmd_dispatch", ok, "command table smoke test failed");
}

static void test_loopback(void)
{
    int st = loopback_check();
    if (st == LB_OK) {
        report("loopback", 1, "");
    } else {
        uart_puts("LW TEST loopback FAIL ");
        uart_puts(st == LB_OPEN ? "PA1 follows nothing (jumper A0-A1 open?)" : "PA1 stuck low (short to ground?)");
        uart_puts("\n");
        fails++;
        wdt_kick();
    }
}

struct test { const char *name; void (*fn)(void); };
static const struct test tests[] = {
    { "crc32", test_crc32 },
    { "ringbuf", test_ringbuf },
    { "systick", test_systick },
    { "cmd_dispatch", test_cmd },
    { "loopback", test_loopback },
};

static int streq(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

int selftest_run_one(const char *name)
{
    for (size_t i = 0; i < sizeof tests / sizeof tests[0]; i++) {
        if (streq(tests[i].name, name)) {
            tests[i].fn();
            return 0;
        }
    }
    return -1;
}

int selftest_run_all(void)
{
    fails = 0;
    for (size_t i = 0; i < sizeof tests / sizeof tests[0]; i++) tests[i].fn();
    return fails;
}
