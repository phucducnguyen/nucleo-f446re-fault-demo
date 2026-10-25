#include "app.h"
#include "board.h"

/* Extra jumper pairs on CN8, each named for the part of a real product it stands in for:
 * A2 (PA4) -> A3 (PB0) is "sensor", A4 (PC1) -> A5 (PC0) is "radio". Same test as loopback.c:
 * drive low then high, read the input with its pull-up. SIM=1 builds report OK.
 * The blue user button stands in for an emergency stop: estop=ON while it is held. */
#define GPIOB_MODER REG32(0x40020400u)
#define GPIOB_PUPDR REG32(0x4002040Cu)
#define GPIOB_IDR   REG32(0x40020410u)
#define GPIOC_MODER REG32(0x40020800u)
#define GPIOC_OTYPER REG32(0x40020804u)
#define GPIOC_PUPDR REG32(0x4002080Cu)
#define GPIOC_IDR   REG32(0x40020810u)
#define GPIOC_BSRR  REG32(0x40020818u)
#define ESTOP_PIN   13u /* B1, the blue user button on PC13: external pull-up, pressed reads low */

#ifndef SIM
static int ready;

static void links_init(void)
{
    RCC_AHB1ENR |= (1u << 0) | (1u << 1) | (1u << 2); /* GPIOA, GPIOB, GPIOC */
    (void)RCC_AHB1ENR;
    GPIOA_MODER = (GPIOA_MODER & ~(3u << 8)) | (1u << 8); /* PA4 output */
    GPIOA_OTYPER &= ~(1u << 4);
    GPIOB_MODER &= ~(3u << 0);                             /* PB0 input */
    GPIOB_PUPDR = (GPIOB_PUPDR & ~(3u << 0)) | (1u << 0);  /* PB0 pull-up */
    GPIOC_MODER = (GPIOC_MODER & ~((3u << 0) | (3u << 2))) | (1u << 2); /* PC1 out, PC0 in */
    GPIOC_OTYPER &= ~(1u << 1);
    GPIOC_PUPDR = (GPIOC_PUPDR & ~(3u << 0)) | (1u << 0);  /* PC0 pull-up */
    ready = 1;
}

static void settle(void)
{
    for (volatile uint32_t n = 0; n < 60u; n++) {
    }
}

/* out_bsrr: the BSRR of the driven pin's port; in_idr: the IDR of the input's port. */
static int pair_check(volatile uint32_t *out_bsrr, uint32_t out_pin, volatile uint32_t *in_idr, uint32_t in_pin)
{
    *out_bsrr = 1u << (out_pin + 16u); /* low */
    settle();
    int low = (*in_idr >> in_pin) & 1u;
    *out_bsrr = 1u << out_pin;         /* high */
    settle();
    int high = (*in_idr >> in_pin) & 1u;
    if (!high) return LB_STUCK;
    if (low) return LB_OPEN;
    return LB_OK;
}
#endif

void links_print_fields(void)
{
    int sensor = LB_OK, radio = LB_OK;
#ifndef SIM
    if (!ready) links_init();
    sensor = pair_check(&GPIOA_BSRR, 4u, &GPIOB_IDR, 0u);
    radio = pair_check(&GPIOC_BSRR, 1u, &GPIOC_IDR, 0u);
#endif
    uart_puts(" sensor=");
    uart_puts(loopback_name(sensor));
    uart_puts(" radio=");
    uart_puts(loopback_name(radio));
    int estop = 0;
#ifndef SIM
    estop = !((GPIOC_IDR >> ESTOP_PIN) & 1u); /* PC13 is an input after reset; never reconfigured */
#endif
    uart_puts(estop ? " estop=ON" : " estop=OFF");
}
