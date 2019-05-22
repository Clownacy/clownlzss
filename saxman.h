#pragma once

#ifndef __cplusplus
#include <stdbool.h>
#endif
#include <stddef.h>

unsigned char* ClownLZSS_SaxmanCompress(unsigned char *data, size_t data_size, size_t *compressed_size, bool header);
unsigned char* ClownLZSS_ModuledSaxmanCompress(unsigned char *data, size_t data_size, size_t *compressed_size, bool header, size_t module_size);
