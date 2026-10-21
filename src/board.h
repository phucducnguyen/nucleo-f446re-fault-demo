/* NUCLEO-F446RE register map (RM0390) and board constants. */
#ifndef BOARD_H
#define BOARD_H
#include <stddef.h>
#include <stdint.h>

#define REG32(a) (*(volatile uint32_t *)(a))

#define SYSCLK_HZ 16000000u /* HSI, no PLL */
#define PCLK1_HZ  16000000u /* APB1 clock, feeds USART2 */
#define UART_BAUD 115200u

/* RCC, base 0x40023800 */
#define RCC_CR      REG32(0x40023800u)
#define RCC_PLLCFGR REG32(0x40023804u)
#define RCC_CFGR    REG32(0x40023808u)
#define RCC_AHB1ENR REG32(0x40023830u) /* bit 0 = GPIOAEN */
#define RCC_APB1ENR REG32(0x40023840u)
#define RCC_CSR     REG32(0x40023874u)

/* GPIOA, base 0x40020000 */
#define GPIOA_MODER REG32(0x40020000u)
#define GPIOA_OTYPER REG32(0x40020004u)
#define GPIOA_PUPDR REG32(0x4002000Cu)
#define GPIOA_IDR   REG32(0x40020010u)
#define GPIOA_AFRL  REG32(0x40020020u)
#define GPIOA_BSRR  REG32(0x40020018u)
#define GPIOA_ODR   REG32(0x40020014u)
#define LED_PIN     5u /* LD2 on PA5 */

/* USART2, base 0x40004400 */
#define USART2_SR  REG32(0x40004400u)
#define USART2_DR  REG32(0x40004404u)
#define USART2_BRR REG32(0x40004408u)
#define USART2_CR1 REG32(0x4000440Cu)
#define USART2_SR_RXNE (1u << 5)
#define USART2_SR_ORE  (1u << 3)

/* IWDG, base 0x40003000 */
#define IWDG_KR  REG32(0x40003000u)
#define IWDG_PR  REG32(0x40003004u)
#define IWDG_RLR REG32(0x40003008u)
#define IWDG_SR  REG32(0x4000300Cu)

/* SysTick and SCB (ARMv7-M) */
#define SYST_CSR  REG32(0xE000E010u)
#define SYST_RVR  REG32(0xE000E014u)
#define SYST_CVR  REG32(0xE000E018u)
#define SCB_VTOR  REG32(0xE000ED08u)
#define SCB_CFSR  REG32(0xE000ED28u)
#define SCB_HFSR  REG32(0xE000ED2Cu)
#define SCB_MMFAR REG32(0xE000ED34u)
#define SCB_BFAR  REG32(0xE000ED38u)

extern const char fw_version[]; /* build/version.c, from FW_VERSION */

#endif
