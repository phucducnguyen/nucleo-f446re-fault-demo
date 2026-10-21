#include "app.h"
#include "board.h"

/* Jumper A0 (PA0) to A1 (PA1) on CN8. SIM=1 builds skip the pins: QEMU has no GPIO model. */

void loopback_init(void)
{
    RCC_AHB1ENR |= 1u;   /* GPIOAEN */
    (void)RCC_AHB1ENR;   /* read back: clock-enable delay (errata) */
#ifndef SIM
    GPIOA_MODER = (GPIOA_MODER & ~((3u << 0) | (3u << 2))) | (1u << 0); /* PA0 output, PA1 input */
    GPIOA_OTYPER &= ~1u;                                                  /* PA0 push-pull */
    GPIOA_PUPDR = (GPIOA_PUPDR & ~(3u << 2)) | (1u << 2);                /* PA1 pull-up */
#endif
}

#ifndef SIM
/* A few microseconds at 16..84 MHz; busy-wait so it does not depend on SysTick. */
static void settle(void)
{
    for (volatile uint32_t n = 0; n < 60u; n++) {
    }
}

static int read_pa1(void) { return (GPIOA_IDR >> 1) & 1u; }
#endif

int loopback_check(void)
{
#ifdef SIM
    return LB_OK;
#else
    GPIOA_BSRR = 1u << 16; /* PA0 low (BR0) */
    settle();
    int low = read_pa1();
    GPIOA_BSRR = 1u << 0;  /* PA0 high (BS0) */
    settle();
    int high = read_pa1();
    if (!high) return LB_STUCK; /* never reads high: shorted to ground */
    if (low) return LB_OPEN;    /* pull-up wins: nothing follows the driver */
    return LB_OK;
#endif
}

const char *loopback_name(int state)
{
    return state == LB_OK ? "OK" : state == LB_OPEN ? "OPEN" : "STUCK";
}
