#include "app.h"
#include "board.h"

void uart_init(void)
{
    RCC_AHB1ENR |= 1u;        /* GPIOAEN */
    RCC_APB1ENR |= 1u << 17;  /* USART2EN */
    (void)RCC_APB1ENR;        /* read back: clock-enable delay (errata) */

    /* PA2 = USART2_TX, PA3 = USART2_RX: alternate function 7 (datasheet AF table). */
    GPIOA_MODER = (GPIOA_MODER & ~((3u << 4) | (3u << 6) | (3u << 10)))
                  | (2u << 4) | (2u << 6) | (1u << 10); /* PA5 output for LD2 */
    GPIOA_AFRL = (GPIOA_AFRL & ~((0xFu << 8) | (0xFu << 12))) | (7u << 8) | (7u << 12);

    /* BRR = fPCLK / baud with 16x oversampling (mantissa<<4 | fraction). */
    USART2_BRR = (PCLK1_HZ + UART_BAUD / 2u) / UART_BAUD;
    USART2_CR1 = (1u << 13) | (1u << 3) | (1u << 2); /* UE | TE | RE */
}

void uart_putc(char c)
{
    while (!(USART2_SR & (1u << 7))) { /* TXE */
    }
    USART2_DR = (uint32_t)(uint8_t)c;
}

void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

void uart_hex32(uint32_t v)
{
    static const char d[] = "0123456789abcdef";
    uart_putc('0');
    uart_putc('x');
    for (int i = 28; i >= 0; i -= 4) uart_putc(d[(v >> i) & 0xFu]);
}

void uart_dec(uint32_t v)
{
    char t[10];
    int n = 0;
    do { t[n++] = (char)('0' + v % 10u); v /= 10u; } while (v);
    while (n) uart_putc(t[--n]);
}

void led_set(int on) { GPIOA_BSRR = on ? (1u << LED_PIN) : (1u << (LED_PIN + 16u)); }
void led_toggle(void) { GPIOA_BSRR = (GPIOA_ODR & (1u << LED_PIN)) ? (1u << (LED_PIN + 16u)) : (1u << LED_PIN); }
