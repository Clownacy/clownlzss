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
	descriptor >>= descriptor_bits_remaining;

	MemoryStream_WriteByte(output_stream, descriptor & 0xFF);
	MemoryStream_WriteByte(output_stream, descriptor >> 8);

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
	--descriptor_bits_remaining;

	descriptor >>= 1;

	if (bit)
		descriptor |= 1 << (TOTAL_DESCRIPTOR_BITS - 1);

	if (descriptor_bits_remaining == 0)
	{
		FlushData();

		descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;
		MemoryStream_Reset(match_stream);
	}
}

static void DoLiteral(unsigned char value, void *user)
{
	(void)user;

	PutDescriptorBit(true);
	PutMatchByte(value);
}

static void DoMatch(size_t distance, size_t length, void *user)
{
	(void)user;

	if (length >= 2 && length <= 5 && distance <= 256)
	{
		PutDescriptorBit(false);
		PutDescriptorBit(false);
		PutDescriptorBit((length - 2) & 2);
		PutDescriptorBit((length - 2) & 1);
		PutMatchByte(-distance);
	}
	else if (length >= 3 && length <= 9)
	{
		PutDescriptorBit(false);
		PutDescriptorBit(true);
		PutMatchByte(-distance & 0xFF);
		PutMatchByte(((-distance >> (8 - 3)) & 0xF8) | ((length - 2) & 7));
	}
	else //if (length >= 3)
	{
		PutDescriptorBit(false);
		PutDescriptorBit(true);
		PutMatchByte(-distance & 0xFF);
		PutMatchByte((-distance >> (8 - 3)) & 0xF8);
		PutMatchByte(length - 1);
	}
}

static unsigned int GetMatchCost(size_t distance, size_t length, void *user)
{
	(void)user;

	if (length >= 2 && length <= 5 && distance <= 256)
		return 8 + 2 + 2;	// Offset byte, length bits, descriptor bits
	else if (length >= 3 && length <= 9)
		return 16 + 2;		// Offset/length bytes, descriptor bits
	else if (length >= 3)
		return 24 + 2;		// Offset/length bytes, descriptor bits
	else
		return 0; 		// In the event a match cannot be compressed
}

CLOWNLZSS_MAKE_FUNCTION(CompressKosinski, unsigned char, 0x100, 0x2000, 8 + 1, DoLiteral, GetMatchCost, DoMatch)

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

			unsigned char *file_buffer = malloc(file_size);
			fread(file_buffer, 1, file_size, file);
			fclose(file);

			output_stream = MemoryStream_Init(0x100);
			match_stream = MemoryStream_Init(0x10);
			descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

			CompressKosinski(file_buffer, file_size, NULL);

			#ifdef DEBUG
			printf("%X - Terminator: %X\n", MemoryStream_GetIndex(output_stream) + MemoryStream_GetIndex(match_stream) + 2, file_pointer - file_buffer);
			#endif

			// Terminator match
			PutDescriptorBit(false);
			PutDescriptorBit(true);
			PutMatchByte(0x00);
			PutMatchByte(0xF0);	// Honestly, I have no idea why this isn't just 0. I guess it's so you can spot it in a hex editor?
			PutMatchByte(0x00);

			FlushData();

			FILE *out_file = fopen("out.kos", "wb");

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
