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

#include "kosinskiplus.h"

#include <assert.h>
#include <stddef.h>

#include "clowncommon.h"

#include "clownlzss.h"
#include "common.h"

#define TOTAL_DESCRIPTOR_BITS 8

typedef struct KosinskiPlusInstance
{
	const ClownLZSS_Callbacks *callbacks;

	size_t descriptor_position;

	unsigned int descriptor;
	unsigned int descriptor_bits_remaining;
} KosinskiPlusInstance;

static size_t GetMatchCost(size_t distance, size_t length, void *user)
{
	(void)user;

	if (length >= 2 && length <= 5 && distance <= 0x100)
		return 2 + 8 + 2;  /* Descriptor bits, offset byte, length bits */
	else if (length >= 3 && length <= 9)
		return 2 + 16;     /* Descriptor bits, offset/length bytes */
	else if (length >= 10)
		return 2 + 16 + 8; /* Descriptor bits, offset bytes, length byte */
	else
		return 0;          /* In the event a match cannot be compressed */
}

static void FindExtraMatches(const unsigned char *data, size_t data_size, size_t offset, ClownLZSS_GraphEdge *node_meta_array, void *user)
{
	(void)data;
	(void)data_size;
	(void)offset;
	(void)node_meta_array;
	(void)user;
}

static CLOWNLZSS_MAKE_COMPRESSION_FUNCTION(CompressData, 1, 0x100 + 8, 0x2000, FindExtraMatches, 1 + 8, GetMatchCost)

static void BeginDescriptorField(KosinskiPlusInstance *instance)
{
	const ClownLZSS_Callbacks* const callbacks = instance->callbacks;

	/* Log the placement of the descriptor field. */
	instance->descriptor_position = callbacks->tell(callbacks->user_data);

	/* Insert a placeholder. */
	callbacks->write(callbacks->user_data, 0);
}

static void FinishDescriptorField(KosinskiPlusInstance *instance)
{
	const ClownLZSS_Callbacks* const callbacks = instance->callbacks;

	/* Back up current position. */
	const size_t current_position = callbacks->tell(callbacks->user_data);

	/* Go back to the descriptor field. */
	callbacks->seek(callbacks->user_data, instance->descriptor_position);

	/* Write the complete descriptor field. */
	callbacks->write(callbacks->user_data, instance->descriptor & 0xFF);

	/* Seek back to where we were before. */
	callbacks->seek(callbacks->user_data, current_position);
}

static void PutDescriptorBit(KosinskiPlusInstance *instance, cc_bool bit)
{
	assert(bit == 0 || bit == 1);

	if (instance->descriptor_bits_remaining == 0)
	{
		instance->descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

		FinishDescriptorField(instance);
		BeginDescriptorField(instance);
	}

	--instance->descriptor_bits_remaining;

	instance->descriptor <<= 1;

	instance->descriptor |= bit;
}

cc_bool ClownLZSS_KosinskiPlusCompress(const unsigned char *data, size_t data_size, const ClownLZSS_Callbacks *callbacks)
{
	KosinskiPlusInstance instance;
	ClownLZSS_Match *matches, *match;
	size_t total_matches;

	/* Set up the state. */
	instance.callbacks = callbacks;
	instance.descriptor = 0;
	instance.descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

	/* Produce a series of LZSS compression matches. */
	if (!CompressData(data, data_size, &matches, &total_matches, &instance))
		return cc_false;

	/* Begin first descriptor field. */
	BeginDescriptorField(&instance);

	/* Produce Kosinski+-formatted data. */
	for (match = matches; match != &matches[total_matches]; ++match)
	{
		if (CLOWNLZSS_MATCH_IS_LITERAL(match))
		{
			PutDescriptorBit(&instance, 1);
			callbacks->write(callbacks->user_data, data[match->destination]);
		}
		else
		{
			const size_t distance = match->destination - match->source;
			const size_t length = match->length;

			if (length >= 2 && length <= 5 && distance <= 0x100)
			{
				PutDescriptorBit(&instance, 0);
				PutDescriptorBit(&instance, 0);
				callbacks->write(callbacks->user_data, -distance & 0xFF);
				PutDescriptorBit(&instance, !!((length - 2) & 2));
				PutDescriptorBit(&instance, !!((length - 2) & 1));
			}
			else if (length >= 3 && length <= 9)
			{
				PutDescriptorBit(&instance, 0);
				PutDescriptorBit(&instance, 1);
				callbacks->write(callbacks->user_data, ((-distance >> (8 - 3)) & 0xF8) | ((10 - length) & 7));
				callbacks->write(callbacks->user_data, -distance & 0xFF);
			}
			else /*if (length >= 10)*/
			{
				PutDescriptorBit(&instance, 0);
				PutDescriptorBit(&instance, 1);
				callbacks->write(callbacks->user_data, (-distance >> (8 - 3)) & 0xF8);
				callbacks->write(callbacks->user_data, -distance & 0xFF);
				callbacks->write(callbacks->user_data, length - 9);
			}
		}
	}

	/* We don't need the matches anymore. */
	free(matches);

	/* Add the terminator match. */
	PutDescriptorBit(&instance, 0);
	PutDescriptorBit(&instance, 1);
	callbacks->write(callbacks->user_data, 0xF0);
	callbacks->write(callbacks->user_data, 0x00);
	callbacks->write(callbacks->user_data, 0x00);

	/* The descriptor field may be incomplete, so move the bits into their proper place. */
	instance.descriptor <<= instance.descriptor_bits_remaining;

	/* Finish last descriptor field. */
	FinishDescriptorField(&instance);

	return cc_true;
}
