#pragma once

#include <stddef.h>

unsigned char* ModuledCompress(unsigned char *data, size_t data_size, size_t *compressed_size, unsigned char* (*function)(unsigned char *data, size_t data_size, size_t *compressed_size), size_t module_size, size_t module_alignment);
