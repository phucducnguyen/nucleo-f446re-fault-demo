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
