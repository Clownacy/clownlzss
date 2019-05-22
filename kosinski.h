#pragma once

#include <stddef.h>

unsigned char* ClownLZSS_KosinskiCompress(unsigned char *data, size_t data_size, size_t *compressed_size);
unsigned char* ClownLZSS_ModuledKosinskiCompress(unsigned char *data, size_t data_size, size_t *compressed_size, size_t module_size);
