#include "app.h"
#include "diag.h"
#include "board.h"

/* ADC1 and the common ADC block (RM0390 section 13). Kept here, not in board.h. */
#define RCC_APB2ENR REG32(0x40023844u) /* bit 8 = ADC1EN */
#define ADC1_SR     REG32(0x40012000u)
#define ADC1_CR2    REG32(0x40012008u)
#define ADC1_SMPR1  REG32(0x4001200Cu)
#define ADC1_SQR1   REG32(0x4001202Cu)
#define ADC1_SQR3   REG32(0x40012034u)
#define ADC1_DR     REG32(0x4001204Cu)
#define ADC_CCR     REG32(0x40012304u) /* bit 23 = TSVREFE */

/* Factory calibration (datasheet DS10693, "Reference voltage" and "Temperature sensor"):
 * raw values taken at VDDA = 3.3 V; TS_CAL1 at 30 C, TS_CAL2 at 110 C. */
#define VREFINT_CAL (*(volatile const uint16_t *)0x1FFF7A2Au)
#define TS_CAL1     (*(volatile const uint16_t *)0x1FFF7A2Cu)
#define TS_CAL2     (*(volatile const uint16_t *)0x1FFF7A2Eu)

#define CH_TEMP    18u /* channel 18 on the F446 (shared with VBAT; VBATE stays off) */
#define CH_VREFINT 17u

#ifndef SIM
static int ready;

/* One conversion; 0 means no reading (bounded wait, so a dead ADC can not hang the loop). */
static uint32_t convert(uint32_t ch)
{
    ADC1_SQR3 = ch;
    ADC1_SR = 0;
    ADC1_CR2 |= 1u << 30; /* SWSTART */
    for (uint32_t n = 0; n < 20000u; n++) {
        if (ADC1_SR & (1u << 1)) return ADC1_DR & 0xFFFu; /* EOC; reading DR clears it */
    }
    return 0;
}

static void print_signed(int32_t v)
{
    if (v < 0) {
        uart_putc('-');
        v = -v;
    }
    uart_dec((uint32_t)v);
}

/* Called lazily from the first HEALTH line, so main.c gains no init line. */
static void health_init(void)
{
    RCC_APB2ENR |= 1u << 8;
    (void)RCC_APB2ENR;         /* read back: clock-enable delay (errata) */
    ADC_CCR |= 1u << 23;       /* TSVREFE; ADCCLK = PCLK2 / 2 = 8 MHz */
    /* 480 cycles (60 us) on both: the sensor needs at least 10 us of sampling. */
    ADC1_SMPR1 = (ADC1_SMPR1 & ~((7u << 21) | (7u << 24))) | (7u << 21) | (7u << 24);
    ADC1_SQR1 = 0;             /* one conversion per start */
    ADC1_CR2 = 1u;             /* ADON */
    for (volatile uint32_t n = 0; n < 2000u; n++) { /* > 10 us sensor start-up at 16 MHz */
    }
    ready = 1;
}
#endif

void health_print_fields(void)
{
#ifndef SIM
    if (!ready) health_init();
    uint32_t vref = convert(CH_VREFINT);
    uint32_t traw = convert(CH_TEMP);
    uint32_t c1 = TS_CAL1, c2 = TS_CAL2;
    if (!vref || !traw || c2 <= c1) return;
    uint32_t vdd = 3300u * VREFINT_CAL / vref;
    /* Scale the reading to the 3.3 V the calibration was taken at, then interpolate. */
    int32_t t3 = (int32_t)(traw * vdd / 3300u);
    int32_t temp = 30 + (t3 - (int32_t)c1) * 80 / (int32_t)(c2 - c1);
    uart_puts(" vdd=");
    uart_dec(vdd);
    uart_puts(" temp=");
    print_signed(temp);
#endif
}
