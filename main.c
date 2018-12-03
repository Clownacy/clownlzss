#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "clownlzss.h"
#include "memory_stream.h"

#define TOTAL_DESCRIPTOR_BITS 16

static MemoryStream *output_stream;
static MemoryStream *match_stream;

static unsigned short descriptor;
static unsigned int descriptor_bits_remaining;

static void FlushData(void)
{
	descriptor <<= descriptor_bits_remaining;

	MemoryStream_WriteByte(output_stream, descriptor >> 8);
	MemoryStream_WriteByte(output_stream, descriptor & 0xFF);

	const size_t match_buffer_size = MemoryStream_GetIndex(match_stream);
	unsigned char *match_buffer = MemoryStream_GetBuffer(match_stream);

	MemoryStream_WriteBytes(output_stream, match_buffer, match_buffer_size);
}

static void PutMatchByte(unsigned char byte)
{
	MemoryStream_WriteByte(match_stream, byte);
}

static void PutDescriptorBit(bool bit)
{
	if (descriptor_bits_remaining == 0)
	{
		FlushData();

		descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;
		MemoryStream_Reset(match_stream);
	}

	--descriptor_bits_remaining;

	descriptor <<= 1;

	descriptor |= bit;
}

static void DoLiteral(unsigned short value, void *user)
{
	(void)user;

	PutDescriptorBit(false);
	PutMatchByte(value & 0xFF);
	PutMatchByte(value >> 8);
}

static void DoMatch(size_t distance, size_t length, void *user)
{
	(void)user;

	PutDescriptorBit(true);
	PutMatchByte(-distance);
	PutMatchByte(length - 1);
}

static unsigned int GetMatchCost(size_t distance, size_t length, void *user)
{
	(void)distance;
	(void)length;
	(void)user;

	return 16 + 1;
}

CLOWNLZSS_MAKE_FUNCTION(CompressComper, unsigned short, 0x100, 0x100, 16 + 1, DoLiteral, GetMatchCost, DoMatch)

int main(int argc, char *argv[])
{
	if (argc > 1)
	{
		FILE *file = fopen(argv[1], "rb");

		if (file)
		{
			fseek(file, 0, SEEK_END);
			const size_t file_size = ftell(file);
			rewind(file);

			unsigned short *file_buffer = malloc(file_size);
			fread(file_buffer, 1, file_size, file);
			fclose(file);

			output_stream = MemoryStream_Init(0x100);
			match_stream = MemoryStream_Init(0x10);
			descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

			CompressComper(file_buffer, file_size / 2, NULL);

			#ifdef DEBUG
			printf("%X - Terminator: %X\n", MemoryStream_GetIndex(output_stream) + MemoryStream_GetIndex(match_stream) + 2, file_pointer - file_buffer);
			#endif

			// Terminator match
			PutDescriptorBit(true);
			PutMatchByte(0x00);
			PutMatchByte(0x00);

			FlushData();

			FILE *out_file = fopen("out.comp", "wb");

			if (out_file)
			{
				unsigned char *out_buffer = MemoryStream_GetBuffer(output_stream);
				const size_t out_size = MemoryStream_GetIndex(output_stream);
				fwrite(out_buffer, out_size, 1, out_file);
				free(out_buffer);
				fclose(out_file);
			}
		}
	}
}
