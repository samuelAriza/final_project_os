#ifndef LZ77_H
#define LZ77_H

#include "../common.h"

int lz77_compress(const file_buffer_t *input, file_buffer_t *output);
int lz77_decompress(const file_buffer_t *input, file_buffer_t *output);

#endif