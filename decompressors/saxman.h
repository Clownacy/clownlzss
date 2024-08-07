#ifndef CLOWNLZSS_DECOMPRESSORS_SAXMAN_H
#define CLOWNLZSS_DECOMPRESSORS_SAXMAN_H

#include "../common.h"

void ClownLZSS_SaxmanDecompress(ClownLZSS_ReadCallback input_callback, const void *input_callback_user_data, const ClownLZSS_Callbacks *output_callbacks, unsigned int compressed_length);

#endif /* CLOWNLZSS_DECOMPRESSORS_SAXMAN_H */
