/*
	(C) 2018-2019 Clownacy

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#include "common.h"

#include <stddef.h>
#include <stdlib.h>

#include "memory_stream.h"

unsigned char* RegularWrapper(unsigned char *data, size_t data_size, size_t *compressed_size, void *user_data, void (*function)(unsigned char *data, size_t data_size, MemoryStream *output_stream, void *user_data))
{
	MemoryStream *output_stream = MemoryStream_Create(false);

	function(data, data_size, output_stream, user_data);

	unsigned char *out_buffer = MemoryStream_GetBuffer(output_stream);

	if (compressed_size)
		*compressed_size = MemoryStream_GetPosition(output_stream);

	MemoryStream_Destroy(output_stream);

	return out_buffer;
}

unsigned char* ModuledCompressionWrapper(unsigned char *data, size_t data_size, size_t *out_compressed_size, void *user_data, void (*function)(unsigned char *data, size_t data_size, MemoryStream *output_stream, void *user_data), size_t module_size, size_t module_alignment)
{
	MemoryStream *output_stream = MemoryStream_Create(false);

	const unsigned short header = (unsigned short)((data_size % module_size) | ((data_size / module_size) << 12));

	MemoryStream_WriteByte(output_stream, header >> 8);
	MemoryStream_WriteByte(output_stream, header & 0xFF);

	for (size_t compressed_size = 0, i = 0; i < data_size; i += module_size)
	{
		if (compressed_size % module_alignment)
			for (unsigned int i = 0; i < module_alignment - (compressed_size % module_alignment); ++i)
				MemoryStream_WriteByte(output_stream, 0);

		const size_t start = MemoryStream_GetPosition(output_stream);
		function(data + i, module_size < data_size - i ? module_size : data_size - i, output_stream, user_data);
		compressed_size = MemoryStream_GetPosition(output_stream) - start;
	}

	unsigned char *out_buffer = MemoryStream_GetBuffer(output_stream);

	if (out_compressed_size)
		*out_compressed_size = MemoryStream_GetPosition(output_stream);

	MemoryStream_Destroy(output_stream);

	return out_buffer;
}
