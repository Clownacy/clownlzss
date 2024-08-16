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

#include "common.h"

#include <stddef.h>
#include <stdlib.h>

typedef struct CallbacksAndBytes
{
	const ClownLZSS_Callbacks *callbacks;
	size_t total_compressed_bytes;
} CallbacksAndBytes;

static void WriteWrapper(void* const user_data, const unsigned char byte)
{
	CallbacksAndBytes* const callbacks_and_bytes = (CallbacksAndBytes*)user_data;

	++callbacks_and_bytes->total_compressed_bytes;
	callbacks_and_bytes->callbacks->write(callbacks_and_bytes->callbacks->user_data, byte);
}

static void SeekWrapper(void* const user_data, const size_t position)
{
	CallbacksAndBytes* const callbacks_and_bytes = (CallbacksAndBytes*)user_data;

	callbacks_and_bytes->callbacks->seek(callbacks_and_bytes->callbacks->user_data, position);
}

static size_t TellWrapper(void* const user_data)
{
	CallbacksAndBytes* const callbacks_and_bytes = (CallbacksAndBytes*)user_data;

	return callbacks_and_bytes->callbacks->tell(callbacks_and_bytes->callbacks->user_data);
}

cc_bool ClownLZSS_ModuledCompressionWrapper(const unsigned char *data, size_t data_size, const ClownLZSS_Callbacks *callbacks, cc_bool (*compression_function)(const unsigned char *data, size_t data_size, const ClownLZSS_Callbacks *callbacks), size_t module_size, size_t module_alignment)
{
	CallbacksAndBytes callbacks_and_bytes;
	ClownLZSS_Callbacks wrapped_callbacks;
	size_t header;
	size_t compressed_size, i;

	callbacks_and_bytes.callbacks = callbacks;
	callbacks_and_bytes.total_compressed_bytes = 0;

	wrapped_callbacks.user_data = &callbacks_and_bytes;
	wrapped_callbacks.write = WriteWrapper;
	wrapped_callbacks.seek = SeekWrapper;
	wrapped_callbacks.tell = TellWrapper;

	header = (data_size % module_size) | ((data_size / module_size) << 12);

	WriteWrapper(&callbacks_and_bytes, (header >> (8 * 1)) & 0xFF);
	WriteWrapper(&callbacks_and_bytes, (header >> (8 * 0)) & 0xFF);

	for (compressed_size = 0, i = 0; i < data_size; i += module_size)
	{
		if (compressed_size % module_alignment != 0)
		{
			size_t j;

			for (j = 0; j < module_alignment - (compressed_size % module_alignment); ++j)
				WriteWrapper(&callbacks_and_bytes, 0);
		}

		callbacks_and_bytes.total_compressed_bytes = 0;

		if (!compression_function(data + i, module_size < data_size - i ? module_size : data_size - i, &wrapped_callbacks))
			return cc_false;

		compressed_size = callbacks_and_bytes.total_compressed_bytes;
	}

	return cc_true;
}
