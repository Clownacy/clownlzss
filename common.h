/*
Copyright (c) 2018-2022 Clownacy

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef CLOWNLZSS_COMMON_H
#define CLOWNLZSS_COMMON_H

#include <stddef.h>

#include "clowncommon/clowncommon.h"

typedef unsigned char (*ClownLZSS_ReadCallback)(void *user_data);

typedef struct ClownLZSS_Callbacks
{
	void *user_data;
	ClownLZSS_ReadCallback read;
	void (*write)(void *user_data, unsigned char byte);
	void (*seek)(void *user_data, size_t position);
	size_t (*tell)(void *user_data);
} ClownLZSS_Callbacks;

#ifdef __cplusplus
extern "C" {
#endif

cc_bool ClownLZSS_ModuledCompressionWrapper(const unsigned char *data, size_t data_size, const ClownLZSS_Callbacks *callbacks, cc_bool (*compression_function)(const unsigned char *data, size_t data_size, const ClownLZSS_Callbacks *callbacks), size_t module_size, size_t module_alignment);

#ifdef __cplusplus
}
#endif

#endif /* CLOWNLZSS_COMMON_H */
