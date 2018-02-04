#ifndef KEYS_H
#define KEYS_H

#include <stdbool.h>
#include <stdint.h>

struct keys_s {
  bool up, down, left, right, a, b, start, select;
};

typedef struct keys_s keys;

typedef int (*keys_cb)(keys*);

#define P15 0x20
#define P14 0x10

#define P13 0x08
#define P12 0x04
#define P11 0x02
#define P10 0x01

void keys_reset(keys *k);

void key_get_raw_output(keys *k, uint8_t *key_in, uint8_t *key_out);

#endif // KEYS_H
