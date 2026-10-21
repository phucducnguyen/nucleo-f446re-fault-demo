#include "app.h"
#include "board.h"

#define LINE_MAX 32u

/* Polled line reader for USART2 RX; no interrupt is ever enabled. A line longer than LINE_MAX
 * or hit by an overrun is dropped whole, silently. */
static char line[LINE_MAX + 1u];
static uint32_t len;
static int drop;

static int starts_with(const char *s, const char *p)
{
    while (*p) if (*s++ != *p++) return 0;
    return 1;
}

static void reply_word(const char *cmd)
{
    uart_puts("LW REPLY ");
    for (uint32_t i = 0; cmd[i] && cmd[i] != ' '; i++)
        uart_putc(cmd[i] >= 0x21 && cmd[i] < 0x7f ? cmd[i] : '?');
}

static void handle(const char *cmd)
{
    if (starts_with(cmd, "ping") && cmd[4] == 0 && cmd_dispatch("ping", "") == 0) {
        uart_puts("LW REPLY ping ok\n");
    } else if (starts_with(cmd, "test ") && selftest_run_one(cmd + 5) == 0) {
        uart_puts("LW REPLY test done\n");
    } else {
        reply_word(cmd);
        uart_puts(" unknown\n"); /* includes a bad or missing test name */
    }
}

void console_poll(void)
{
    uint32_t sr = USART2_SR;
    if (!(sr & (USART2_SR_RXNE | USART2_SR_ORE))) return;
    char c = (char)USART2_DR; /* SR read then DR read also clears ORE */
    if (sr & USART2_SR_ORE) drop = 1; /* bytes were lost: this line cannot be trusted */
    if (c == '\r' || c == '\n') {
        if (len && !drop) {
            line[len] = 0;
            handle(line);
        }
        len = 0;
        drop = 0;
    } else if (len < LINE_MAX) {
        line[len++] = c;
    } else {
        drop = 1; /* too long */
    }
}
