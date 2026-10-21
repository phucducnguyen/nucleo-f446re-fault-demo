#include "app.h"
#include "board.h"

/* Run the core at 84 MHz: HSI 16 MHz / PLLM 16 * PLLN 336 / PLLP 4 = 84 MHz (RM0390 6.3.2).
 * APB1 is limited to 45 MHz, so it runs at SYSCLK/2. Each wait is bounded so a clock that
 * never comes ready (or an emulator without an RCC model) falls through instead of hanging. */
#define FLASH_ACR REG32(0x40023C00u)

static int wait_for(volatile uint32_t *reg, uint32_t mask, uint32_t want)
{
    for (uint32_t n = 0; n < 100000u; n++)
        if ((*reg & mask) == want) return 1;
    return 0;
}

void clock_init(void)
{
    /* 2 wait states for 84 MHz at 2.7..3.6 V, plus prefetch, I-cache, D-cache. */
    FLASH_ACR = 2u | (1u << 8) | (1u << 9) | (1u << 10);
    if (!wait_for(&FLASH_ACR, 7u, 2u)) return;

    RCC_PLLCFGR = 16u              /* PLLM */
                  | (336u << 6)    /* PLLN */
                  | (1u << 16)     /* PLLP = /4 */
                  | (7u << 24)     /* PLLQ */
                  | (2u << 28);    /* PLLR (reset value) */
    RCC_CFGR = (RCC_CFGR & ~(7u << 10)) | (4u << 10); /* PPRE1 = /2 */
    RCC_CR |= 1u << 24;                               /* PLLON */
    if (!wait_for(&RCC_CR, 1u << 25, 1u << 25)) return; /* PLLRDY */
    RCC_CFGR = (RCC_CFGR & ~3u) | 2u;                 /* SW = PLL */
    wait_for(&RCC_CFGR, 3u << 2, 2u << 2);            /* SWS = PLL */
}
