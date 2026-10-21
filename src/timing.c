#include "app.h"
#include "board.h"

static volatile uint32_t ticks;

void SysTick_Handler(void) { ticks++; }

void systick_init(void)
{
    SYST_RVR = SYSCLK_HZ / 1000u - 1u; /* 1 ms */
    SYST_CVR = 0;
    SYST_CSR = 7u; /* ENABLE | TICKINT | CLKSOURCE=processor clock */
}

uint32_t millis(void) { return ticks; }
