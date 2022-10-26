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

#include "saxman.h"

#include <assert.h>
#include <stddef.h>

#include "clowncommon.h"

#include "clownlzss.h"
#include "common.h"

#define TOTAL_DESCRIPTOR_BITS 8

typedef struct SaxmanInstance
{
	const ClownLZSS_Callbacks *callbacks;

	size_t descriptor_position;

	unsigned int descriptor;
	unsigned int descriptor_bits_remaining;
} SaxmanInstance;

static size_t GetMatchCost(size_t distance, size_t length, void *user)
{
	(void)distance;
	(void)length;
	(void)user;

	if (length >= 3)
		return 1 + 16;	/* Descriptor bit, offset/length bits */
	else
		return 0;
}

static void FindExtraMatches(const unsigned char *data, size_t data_size, size_t offset, ClownLZSS_GraphEdge *node_meta_array, void *user)
{
	(void)user;

	if (offset < 0x1000)
	{
		size_t i;

		const size_t max_read_ahead = CLOWNLZSS_MIN(0x12, data_size - offset);

		for (i = 0; i < max_read_ahead && data[offset + i] == 0; ++i)
		{
			const unsigned int cost = GetMatchCost(0, i + 1, user);

			if (cost && node_meta_array[offset + i + 1].u.cost > node_meta_array[offset].u.cost + cost)
			{
				node_meta_array[offset + i + 1].u.cost = node_meta_array[offset].u.cost + cost;
				node_meta_array[offset + i + 1].previous_node_index = offset;
				node_meta_array[offset + i + 1].match_offset = 0xFFF;
			}
		}
	}
}

static CLOWNLZSS_MAKE_COMPRESSION_FUNCTION(CompressData, 1, 0x12, 0x1000, FindExtraMatches, 1 + 8, GetMatchCost)

static void BeginDescriptorField(SaxmanInstance *instance)
{
	const ClownLZSS_Callbacks* const callbacks = instance->callbacks;

	/* Log the placement of the descriptor field. */
	instance->descriptor_position = callbacks->tell(callbacks->user_data);

	/* Insert a placeholder. */
	callbacks->write(callbacks->user_data, 0);
}

static void FinishDescriptorField(SaxmanInstance *instance)
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

static void PutDescriptorBit(SaxmanInstance *instance, cc_bool bit)
{
	assert(bit == 0 || bit == 1);

	if (instance->descriptor_bits_remaining == 0)
	{
		instance->descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

		FinishDescriptorField(instance);
		BeginDescriptorField(instance);
	}

	--instance->descriptor_bits_remaining;

	instance->descriptor >>= 1;

	if (bit)
		instance->descriptor |= 1 << (TOTAL_DESCRIPTOR_BITS - 1);
}

static cc_bool SaxmanCompress(const unsigned char *data, size_t data_size, const ClownLZSS_Callbacks *callbacks, cc_bool header)
{
	SaxmanInstance instance;
	ClownLZSS_Match *matches, *match;
	size_t total_matches;
	size_t header_position, end_position;

	/* Set up the state. */
	instance.callbacks = callbacks;
	instance.descriptor = 0;
	instance.descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

	/* Produce a series of LZSS compression matches. */
	if (!CompressData(data, data_size, &matches, &total_matches, &instance))
		return cc_false;

	if (header)
	{
		/* Track the location of the header... */
		header_position = callbacks->tell(callbacks->user_data);

		/* ...and insert a placeholder there. */
		callbacks->write(callbacks->user_data, 0);
		callbacks->write(callbacks->user_data, 0);
	}

	/* Begin first descriptor field. */
	BeginDescriptorField(&instance);

	/* Produce Saxman-formatted data. */
	for (match = matches; match != &matches[total_matches]; ++match)
	{
		if (CLOWNLZSS_MATCH_IS_LITERAL(match))
		{
			PutDescriptorBit(&instance, 1);
			callbacks->write(callbacks->user_data, data[match->destination]);
		}
		else
		{
			const size_t offset = match->source - 0x12;
			const size_t length = match->length;

			PutDescriptorBit(&instance, 0);
			callbacks->write(callbacks->user_data, offset & 0xFF);
			callbacks->write(callbacks->user_data, ((offset & 0xF00) >> 4) | (length - 3));
		}
	}

	/* We don't need the matches anymore. */
	free(matches);

	/* The descriptor field may be incomplete, so move the bits into their proper place. */
	instance.descriptor >>= instance.descriptor_bits_remaining;

	/* Finish last descriptor field. */
	FinishDescriptorField(&instance);

	if (header)
	{
		/* Grab the current position for later. */
		end_position = callbacks->tell(callbacks->user_data);

		/* Rewind to the header... */
		callbacks->seek(callbacks->user_data, header_position);

		/* ...and complete it. */
		callbacks->write(callbacks->user_data, ((end_position - header_position - 2) >> (8 * 0)) & 0xFF);
		callbacks->write(callbacks->user_data, ((end_position - header_position - 2) >> (8 * 1)) & 0xFF);

		/* Seek back to the end of the file just as the caller might expect us to do. */
		callbacks->seek(callbacks->user_data, end_position);
	}

	return cc_true;
}

cc_bool ClownLZSS_SaxmanCompressWithHeader(const unsigned char *data, size_t data_size, const ClownLZSS_Callbacks *callbacks)
{
	return SaxmanCompress(data, data_size, callbacks, cc_true);
}

cc_bool ClownLZSS_SaxmanCompressWithoutHeader(const unsigned char *data, size_t data_size, const ClownLZSS_Callbacks *callbacks)
{
	return SaxmanCompress(data, data_size, callbacks, cc_false);
}
