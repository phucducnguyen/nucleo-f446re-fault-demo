#include "app.h"
#include "diag.h"

/* SRAM bounds (link.ld): a stack word outside them is never read, a bad read would lock up. */
#define SRAM_START 0x20000000u
#define SRAM_END   0x20020000u
#define STACK_LINES 8u

static void hex(const char *name, uint32_t v)
{
    uart_puts(name);
    uart_hex32(v);
}

/* CFSR bit names (ARMv7-M ARM B3.2.15) and HFSR (B3.2.16). */
static const struct { uint8_t bit; const char *name; } cfsr_bits[] = {
    { 0, "IACCVIOL" }, { 1, "DACCVIOL" }, { 3, "MUNSTKERR" }, { 4, "MSTKERR" },
    { 5, "MLSPERR" }, { 7, "MMARVALID" }, { 8, "IBUSERR" }, { 9, "PRECISERR" },
    { 10, "IMPRECISERR" }, { 11, "UNSTKERR" }, { 12, "STKERR" }, { 13, "LSPERR" },
    { 15, "BFARVALID" }, { 16, "UNDEFINSTR" }, { 17, "INVSTATE" }, { 18, "INVPC" },
    { 19, "NOCP" }, { 24, "UNALIGNED" }, { 25, "DIVBYZERO" },
};

void crashdump_print(const uint32_t *frame, uint32_t sp, uint32_t cfsr, uint32_t hfsr)
{
    uart_puts("LW DUMP reg");
    hex(" r0=", frame[0]); hex(" r1=", frame[1]); hex(" r2=", frame[2]); hex(" r3=", frame[3]);
    hex(" r12=", frame[4]); hex(" lr=", frame[5]); hex(" pc=", frame[6]); hex(" psr=", frame[7]);
    uart_puts("\n");
    wdt_kick();

    uint32_t a = sp & ~3u;
    if (a < SRAM_START || a >= SRAM_END) {
        hex("LW DUMP stack bad sp=", sp);
        uart_puts("\n");
    } else {
        for (uint32_t line = 0; line < STACK_LINES && a + 16u <= SRAM_END; line++, a += 16u) {
            const volatile uint32_t *w = (const volatile uint32_t *)a;
            uart_puts("LW DUMP stack ");
            uart_hex32(a);
            for (int i = 0; i < 4; i++) {
                uart_putc(' ');
                uart_hex32(w[i]);
            }
            uart_puts("\n");
            wdt_kick();
        }
    }

    uart_puts("LW DUMP cause");
    int any = 0;
    if (hfsr & (1u << 1)) { uart_puts(" VECTTBL"); any = 1; }
    if (hfsr & (1u << 30)) { uart_puts(" FORCED"); any = 1; }
    for (size_t i = 0; i < sizeof cfsr_bits / sizeof cfsr_bits[0]; i++) {
        if (cfsr & (1u << cfsr_bits[i].bit)) {
            uart_putc(' ');
            uart_puts(cfsr_bits[i].name);
            any = 1;
        }
    }
    if (!any) uart_puts(" NONE");
    uart_puts("\nLW DUMP end\n");
    wdt_kick();
}
