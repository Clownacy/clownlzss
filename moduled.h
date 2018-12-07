#pragma once

#include <stddef.h>

#include "memory_stream.h"

unsigned char* ModuledCompress(unsigned char *data, size_t data_size, size_t *out_compressed_size, void (*function)(unsigned char *data, size_t data_size, MemoryStream *output_stream), size_t module_size, size_t module_alignment);
