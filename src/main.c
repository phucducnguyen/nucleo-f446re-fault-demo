#include "app.h"
#include "board.h"

static void print_reset_cause(uint32_t csr)
{
    /* RCC_CSR flags, bit order of the BOOT line: LPWR WWDG IWDG SFT POR PIN BOR (bits 31..25). */
    static const char *const names[] = { "LPWR", "WWDG", "IWDG", "SFT", "POR", "PIN", "BOR" };
    int first = 1;
    for (int i = 0; i < 7; i++) {
        if (csr & (1u << (31 - i))) {
            if (!first) uart_putc(',');
            uart_puts(names[i]);
            first = 0;
        }
    }
    if (first) uart_puts("NONE");
}

static void banner(uint32_t csr)
{
    uart_puts("\nBench test firmware " APP_VERSION
#ifdef SIM
              " (SIM build: loopback not real)"
#endif
              "\n");
    uart_puts("LW BOOT fw=");
    uart_puts(fw_version);
    uart_puts(" reset=");
    print_reset_cause(csr);
    uart_puts(" csr=");
    uart_hex32(csr);
    uart_puts("\n");
}

int main(void)
{
    uint32_t csr = RCC_CSR; /* read before anything can disturb it */

    clock_init();
    systick_init();
    wdt_start();
    fault_init();
    uart_init();
    loopback_init();
    banner(csr);
    RCC_CSR |= 1u << 24; /* RMVF: clear the reset flags */

    sensor_calibrate();

    int fails = selftest_run_all();
    uart_puts(fails ? "LW RESULT FAIL\n" : "LW RESULT PASS\n");

    uint32_t led_half = fails ? 100u : 500u; /* 5 Hz fast blink on FAIL, 1 Hz on PASS */
    uint32_t now = millis(), next_led = now + led_half, next_alive = now + 1000u, n = 0;
    for (;;) {
        wdt_kick();
        console_poll();
        now = millis();
        if ((int32_t)(now - next_led) >= 0) {
            led_toggle();
            next_led += led_half;
        }
        if ((int32_t)(now - next_alive) >= 0) {
            uart_puts("LW ALIVE ");
            uart_dec(++n);
            uart_puts("\n");
            uart_puts("LW HEALTH ");
            uart_dec(n);
            uart_puts(" loopback=");
            uart_puts(loopback_name(loopback_check()));
            uart_puts("\n");
            next_alive += 1000u;
        }
    }
}
