#pragma once

#include <stddef.h>

unsigned char* ComperCompress(unsigned char *data, size_t data_size, size_t *compressed_size);
unsigned char* ModuledComperCompress(unsigned char *data, size_t data_size, size_t *compressed_size, size_t module_size);
