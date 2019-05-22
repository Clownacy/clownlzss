#pragma once

#include <stddef.h>

unsigned char* ClownLZSS_ComperCompress(unsigned char *data, size_t data_size, size_t *compressed_size);
unsigned char* ClownLZSS_ModuledComperCompress(unsigned char *data, size_t data_size, size_t *compressed_size, size_t module_size);
