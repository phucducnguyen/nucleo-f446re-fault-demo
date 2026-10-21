#include "app.h"
#include "board.h"

/* LSI is about 32 kHz (17..47 kHz over temperature, RM0390 / datasheet):
 * /32 prescaler gives ~1 kHz, reload 800 gives ~0.8 s nominal. */
void wdt_start(void)
{
    IWDG_KR = 0xCCCCu; /* start; the LSI is forced on */
    IWDG_KR = 0x5555u; /* unlock PR and RLR */
    IWDG_PR = 3u;      /* divider /32 */
    IWDG_RLR = 800u;
    /* Wait for PVU/RVU to clear; bounded so QEMU (no IWDG model) cannot hang. */
    for (uint32_t n = 0; (IWDG_SR & 3u) && n < 200000u; n++) {
    }
    IWDG_KR = 0xAAAAu; /* reload */
}

void wdt_kick(void) { IWDG_KR = 0xAAAAu; }
