#ifndef APP_H
#define APP_H
#include <stdint.h>
#include <stddef.h>

#define APP_VERSION "1.0.0"

/* timing.c */
void systick_init(void);
uint32_t millis(void);

/* wdt.c */
void wdt_start(void);
void wdt_kick(void);

/* clock.c */
void clock_init(void);

/* uart.c */
void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);
void uart_hex32(uint32_t v);
void uart_dec(uint32_t v);
void led_set(int on);
void led_toggle(void);

/* crc32.c */
uint32_t crc32(const void *data, size_t len);

/* ringbuf.c */
#define RB_SIZE 8u
struct ringbuf { uint8_t buf[RB_SIZE]; uint32_t head, tail, count; };
void rb_init(struct ringbuf *rb);
int rb_push(struct ringbuf *rb, uint8_t b);
int rb_pop(struct ringbuf *rb, uint8_t *b);

/* sensor.c */
void sensor_calibrate(void);

/* cmd.c */
size_t cmd_count(void);
const char *cmd_name(size_t i);
int cmd_dispatch(const char *name, const char *arg);

/* selftest.c: prints LW TEST lines, returns the number of failures */
int selftest_run_all(void);
int selftest_run_one(const char *name); /* 0 = ran (line printed), -1 = unknown name */

/* loopback.c: PA0 (out) jumpered to PA1 (in), see README "Live health" */
enum { LB_OK, LB_OPEN, LB_STUCK };
void loopback_init(void);
int loopback_check(void);
const char *loopback_name(int state);

/* console.c: polled USART2 RX line commands */
void console_poll(void);

/* fault.c */
void fault_init(void);
#endif
