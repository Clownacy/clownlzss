#pragma once

#include <stddef.h>

unsigned char* SaxmanCompress(unsigned char *data, size_t data_size, size_t *compressed_size);
unsigned char* ModuledSaxmanCompress(unsigned char *data, size_t data_size, size_t *compressed_size, size_t module_size);
