#include "moduled.h"

#include <stddef.h>
#include <stdlib.h>

#include "memory_stream.h"

unsigned char* ModuledCompress(unsigned char *data, size_t data_size, size_t *compressed_size, unsigned char* (*function)(unsigned char *data, size_t data_size, size_t *compressed_size), size_t module_size, size_t module_alignment)
{
	MemoryStream *output_stream = MemoryStream_Create(0x1000, false);

	const unsigned short header = (data_size % module_size) | ((data_size / module_size) << 12);

	MemoryStream_WriteByte(output_stream, header >> 8);
	MemoryStream_WriteByte(output_stream, header & 0xFF);

	for (size_t compressed_size = 0, i = 0; i < data_size; i += module_size)
	{
		if (compressed_size % module_alignment)
			for (unsigned int i = 0; i < module_alignment - (compressed_size % module_alignment); ++i)
				MemoryStream_WriteByte(output_stream, 0);

		unsigned char *compressed_buffer = function(data + i, module_size < data_size - i ? module_size : data_size - i, &compressed_size);

		MemoryStream_WriteBytes(output_stream, compressed_buffer, compressed_size);

		free(compressed_buffer);
	}

	unsigned char *out_buffer = MemoryStream_GetBuffer(output_stream);

	if (compressed_size)
		*compressed_size = MemoryStream_GetIndex(output_stream);

	MemoryStream_Destroy(output_stream);

	return out_buffer;
}
