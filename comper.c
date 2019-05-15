#include "comper.h"

#include <stdbool.h>
#include <stddef.h>

#include "clownlzss.h"
#include "common.h"
#include "memory_stream.h"

#define TOTAL_DESCRIPTOR_BITS 16

typedef struct Instance
{
	MemoryStream *output_stream;
	MemoryStream *match_stream;

	unsigned short descriptor;
	unsigned int descriptor_bits_remaining;
} Instance;

static void FlushData(Instance *instance)
{
	MemoryStream_WriteByte(instance->output_stream, instance->descriptor >> 8);
	MemoryStream_WriteByte(instance->output_stream, instance->descriptor & 0xFF);

	const size_t match_buffer_size = MemoryStream_GetPosition(instance->match_stream);
	unsigned char *match_buffer = MemoryStream_GetBuffer(instance->match_stream);

	MemoryStream_WriteBytes(instance->output_stream, match_buffer, match_buffer_size);
}

static void PutMatchByte(Instance *instance, unsigned char byte)
{
	MemoryStream_WriteByte(instance->match_stream, byte);
}

static void PutDescriptorBit(Instance *instance, bool bit)
{
	if (instance->descriptor_bits_remaining == 0)
	{
		FlushData(instance);

		instance->descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;
		MemoryStream_Rewind(instance->match_stream);
	}

	--instance->descriptor_bits_remaining;

	instance->descriptor <<= 1;

	instance->descriptor |= bit;
}

static void DoLiteral(unsigned short value, void *user)
{
	Instance *instance = (Instance*)user;

	PutDescriptorBit(instance, 0);
	PutMatchByte(instance, value & 0xFF);
	PutMatchByte(instance, value >> 8);
}

static void DoMatch(size_t distance, size_t length, size_t offset, void *user)
{
	(void)offset;

	Instance *instance = (Instance*)user;

	PutDescriptorBit(instance, 1);
	PutMatchByte(instance, -distance);
	PutMatchByte(instance, length - 1);
}

static unsigned int GetMatchCost(size_t distance, size_t length, void *user)
{
	(void)distance;
	(void)length;
	(void)user;

	return 1 + 16;	// Descriptor bit, offset/length bytes
}

static void FindExtraMatches(unsigned short *data, size_t data_size, size_t offset, ClownLZSS_GraphEdge *node_meta_array, void *user)
{
	(void)data;
	(void)data_size;
	(void)offset;
	(void)node_meta_array;
	(void)user;
}

static CLOWNLZSS_MAKE_COMPRESSION_FUNCTION(CompressData, unsigned short, 0x100, 0x100, FindExtraMatches, 1 + 16, DoLiteral, GetMatchCost, DoMatch)

static void ComperCompressStream(unsigned char *data, size_t data_size, MemoryStream *output_stream, void *user_data)
{
	(void)user_data;

	Instance instance;
	instance.output_stream = output_stream;
	instance.match_stream = MemoryStream_Create(0x10, true);
	instance.descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

	CompressData((unsigned short*)data, data_size / sizeof(unsigned short), &instance);

	// Terminator match
	PutDescriptorBit(&instance, 1);
	PutMatchByte(&instance, 0);
	PutMatchByte(&instance, 0);

	instance.descriptor <<= instance.descriptor_bits_remaining;
	FlushData(&instance);

	MemoryStream_Destroy(instance.match_stream);
}

unsigned char* ComperCompress(unsigned char *data, size_t data_size, size_t *compressed_size)
{
	return RegularWrapper(data, data_size, compressed_size, NULL, ComperCompressStream);
}

unsigned char* ModuledComperCompress(unsigned char *data, size_t data_size, size_t *compressed_size, size_t module_size)
{
	return ModuledCompressionWrapper(data, data_size, compressed_size, NULL, ComperCompressStream, module_size, 1);
}
