#include "app.h"

void rb_init(struct ringbuf *rb) { rb->head = rb->tail = rb->count = 0; }

int rb_push(struct ringbuf *rb, uint8_t b)
{
    if (rb->count == RB_SIZE) return -1;
    rb->buf[rb->head] = b;
    rb->head = (rb->head + 1u) % RB_SIZE;
    rb->count++;
    return 0;
}

int rb_pop(struct ringbuf *rb, uint8_t *b)
{
    if (rb->count == 0) return -1;
    *b = rb->buf[rb->tail];
    rb->tail = (rb->tail + 1u) % RB_SIZE;
    rb->count--;
    return 0;
}

/* The next four queued bytes as one little-endian word, without popping them. */
uint32_t rb_peek32(const struct ringbuf *rb)
{
    return *(const uint32_t *)&rb->buf[rb->tail];
}
