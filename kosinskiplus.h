#pragma once

#include <stddef.h>

unsigned char* KosinskiPlusCompress(unsigned char *data, size_t data_size, size_t *compressed_size);
