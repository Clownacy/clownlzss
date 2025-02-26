/*
Copyright (c) 2018-2024 Clownacy

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

#ifndef CLOWNLZSS_COMPRESSORS_RAGE_H
#define CLOWNLZSS_COMPRESSORS_RAGE_H

#include <algorithm>
#include <utility>

#include "clownlzss.h"
#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace Rage
		{
			inline std::size_t GetMatchCost([[maybe_unused]] const std::size_t distance, [[maybe_unused]] const std::size_t length, [[maybe_unused]] void* const user)
			{
				if (length >= 4)
					return (2 + (((length - 4 - 3) + (0x1F - 1)) / 0x1F)) * 8;
				else
					return 0;
			}

			inline void FindExtraMatches(const unsigned char* const data, const std::size_t data_size, const std::size_t offset, ClownLZSS_GraphEdge* const node_meta_array, [[maybe_unused]] void* const user)
			{
				std::size_t max_read_ahead;
				std::size_t k;

				// Look for RLE-matches.
				max_read_ahead = std::min<std::size_t>(0xFFF + 4, data_size - offset);

				for (k = 0; k < max_read_ahead; ++k)
				{
					if (data[offset + k] == data[offset])
					{
						unsigned int cost;

						if (k + 1 < 4)
							cost = 0;
						else
							cost = ((k + 1 - 4 > 0xF ? 2 : 1) + 1) * 8;

						if (cost && node_meta_array[offset + k + 1].u.cost > node_meta_array[offset].u.cost + cost)
						{
							node_meta_array[offset + k + 1].u.cost = node_meta_array[offset].u.cost + cost;
							node_meta_array[offset + k + 1].previous_node_index = offset;
							node_meta_array[offset + k + 1].match_offset = 0xFFFFFF00 | data[offset];	// Horrible hack, like the rest of this compressor.
						}
					}
					else
						break;
				}

				// Add uncompressed runs.
				max_read_ahead = std::min<std::size_t>(0x1FFF, data_size - offset);

				for (k = 0; k < max_read_ahead; ++k)
				{
					const unsigned int cost = (k + 1 + (k + 1 > 0x1F ? 2 : 1)) * 8;

					if (cost && node_meta_array[offset + k + 1].u.cost > node_meta_array[offset].u.cost + cost)
					{
						node_meta_array[offset + k + 1].u.cost = node_meta_array[offset].u.cost + cost;
						node_meta_array[offset + k + 1].previous_node_index = offset;
						node_meta_array[offset + k + 1].match_offset = offset;	// Points at itself.
					}
				}
			}

			template<typename T>
			bool Compress(const unsigned char* const data, const std::size_t data_size, CompressorOutput<T> &output)
			{
				// Produce a series of LZSS compression matches.
				// Yes, the distance really is 1 lower than usual.
				ClownLZSS::Matches matches;
				std::size_t total_matches;
				if (!ClownLZSS::FindOptimalMatches(-1, 0xFFFFFFFF/*dictionary-matches can be infinite*/, 0x1FFF, FindExtraMatches, 0xFFFFFFF/*dummy*/, GetMatchCost, data, 1, data_size, &matches, &total_matches, nullptr))
					return false;

				// Track the location of the header...
				const auto header_position = output.Tell();

				// ...and insert a placeholder there.
				output.WriteLE16(0);

				// Produce Rage-formatted data.
				for (ClownLZSS_Match *match = &matches[0]; match != &matches[total_matches]; ++match)
				{
					const std::size_t distance = match->destination - match->source;
					const std::size_t offset = match->source;

					std::size_t length;

					length = match->length;

					if (distance == 0)
					{
						std::size_t i;

						// Uncompressed run.
						if (length > 0x1F)
						{
							output.Write(0x20 | ((length >> 8) & 0x1F));
							output.Write(length & 0xFF);
						}
						else
						{
							output.Write(length);
						}

						for (i = 0; i < length; ++i)
							output.Write(data[offset + i]);
					}
					else if ((offset & 0xFFFFFF00) == 0xFFFFFF00)
					{
						// RLE-match.
						length -= 4;

						if (length > 0xF)
						{
							output.Write(0x40 | 0x10 | ((length >> 8) & 0xF));
							output.Write(length & 0xFF);
						}
						else
						{
							output.Write(0x40 | (length & 0xF));
						}

						output.Write(offset & 0xFF);
					}
					else
					{
						std::size_t thing;

						// Dictionary-match.
						length -= 4;

						// The first match can only encode 7 bytes.
						thing = length > 3 ? 3 : length;

						output.Write(0x80 | (thing << 5) | ((distance >> 8) & 0x1F));
						output.Write(distance & 0xFF);

						length -= thing;

						// If there are still more bytes in this match, do them in blocks of 0x1F bytes.
						while (length != 0)
						{
							thing = length > 0x1F ? 0x1F : length;

							output.Write(0x60 | thing);
							length -= thing;
						}
					}
				}

				// Grab the current position for later.
				const auto end_position = output.Tell();

				// Rewind to the header...
				output.Seek(header_position);

				// ...and complete it.
				output.WriteLE16(end_position - header_position);

				// Seek back to the end of the file just as the caller might expect us to do.
				output.Seek(end_position);

				return true;
			}
		}
	}

	template<typename T>
	bool RageCompress(const unsigned char* const data, const std::size_t data_size, T &&output)
	{
		using namespace Internal;

		CompressorOutput output_wrapped(std::forward<T>(output));
		return Rage::Compress(data, data_size, output_wrapped);
	}

	template<typename T>
	bool ModuledRageCompress(const unsigned char* const data, const std::size_t data_size, T &&output, const std::size_t module_size)
	{
		using namespace Internal;

		CompressorOutput output_wrapped(std::forward<T>(output));
		return ModuledCompressionWrapper<2, Endian::Big>(data, data_size, output_wrapped, Rage::Compress, module_size, 2);
	}
}

#endif // CLOWNLZSS_COMPRESSORS_RAGE_H
