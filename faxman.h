#pragma once

#include <stddef.h>

unsigned char* ClownLZSS_FaxmanCompress(unsigned char *data, size_t data_size, size_t *compressed_size);
unsigned char* ClownLZSS_ModuledFaxmanCompress(unsigned char *data, size_t data_size, size_t *compressed_size, size_t module_size);
