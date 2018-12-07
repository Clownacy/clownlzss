#pragma once

#include <stddef.h>

unsigned char* KosinskiPlusCompress(unsigned char *data, size_t data_size, size_t *compressed_size);
unsigned char* ModuledKosinskiPlusCompress(unsigned char *data, size_t data_size, size_t *compressed_size, size_t module_size);
