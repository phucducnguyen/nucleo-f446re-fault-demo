#include "app.h"
#include "board.h"

/* Wait one SysTick period by polling COUNTFLAG (RM/PM0214 SYST_CSR bit 16).
 * The SysTick interrupt cannot preempt a fault handler, so millis() would freeze here. */
static void tick_wait(void)
{
    (void)SYST_CSR; /* a read clears COUNTFLAG */
    for (uint32_t n = 0; !(SYST_CSR & (1u << 16)) && n < 4000000u; n++) {
    }
}

static void field(const char *name, uint32_t v)
{
    uart_puts(name);
    uart_hex32(v);
}

/* frame = stacked r0,r1,r2,r3,r12,lr,pc,psr (ARMv7-M exception entry). */
void __attribute__((used)) fault_c(const uint32_t *frame)
{
    uint32_t pc = frame[6], lr = frame[5], psr = frame[7];
    uint32_t sp = (uint32_t)(frame + 8) + ((psr & (1u << 9)) ? 4u : 0u); /* undo stack realign */
    uint32_t cfsr = SCB_CFSR, hfsr = SCB_HFSR, bfar = SCB_BFAR, mmfar = SCB_MMFAR;
    uint32_t ms = 0;

    for (;;) {
        if (ms % 1000u == 0) {
            uart_puts("LW HARDFAULT");
            field(" pc=", pc); field(" lr=", lr); field(" psr=", psr);
            field(" cfsr=", cfsr); field(" hfsr=", hfsr); field(" bfar=", bfar);
            field(" mmfar=", mmfar); field(" sp=", sp);
            uart_puts("\n");
        }
        if (ms % 100u == 0) led_toggle(); /* 5 Hz */
        wdt_kick();                       /* keep the fault visible, no watchdog reset */
        tick_wait();
        ms++;
    }
}

/* Pick MSP or PSP from EXC_RETURN bit 2 and hand the frame to C. */
#define FAULT_STUB(name) \
    void __attribute__((naked)) name(void) \
    { \
        __asm volatile("tst lr, #4\n\t" \
                       "ite eq\n\t" \
                       "mrseq r0, msp\n\t" \
                       "mrsne r0, psp\n\t" \
                       "b fault_c\n\t"); \
    }

FAULT_STUB(HardFault_Handler)
FAULT_STUB(MemManage_Handler)
FAULT_STUB(BusFault_Handler)
FAULT_STUB(UsageFault_Handler)

/* The Usage/Bus/MemManage faults (SHCSR bits 18..16) are left disabled on purpose: they
 * escalate to HardFault with HFSR.FORCED, so every fault prints the same dump. */
void fault_init(void) {}
