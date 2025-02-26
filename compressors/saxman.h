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

#ifndef CLOWNLZSS_COMPRESSORS_SAXMAN_H
#define CLOWNLZSS_COMPRESSORS_SAXMAN_H

#include <algorithm>
#include <utility>

#include "../bitfield.h"
#include "clownlzss.h"
#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace Saxman
		{
			template<typename T>
			using DescriptorFieldWriter = BitField::DescriptorFieldWriter<1, BitField::WriteWhen::BeforePush, BitField::PushWhere::High, BitField::Endian::Little, T>;

			inline std::size_t GetMatchCost([[maybe_unused]] const std::size_t distance, [[maybe_unused]] const std::size_t length, [[maybe_unused]] void* const user)
			{
				if (length >= 3)
					return 1 + 16;	// Descriptor bit, offset/length bits.
				else
					return 0;
			}

			inline void FindExtraMatches(const unsigned char* const data, const std::size_t total_values, const std::size_t offset, ClownLZSS_GraphEdge* const node_meta_array, [[maybe_unused]] void* const user)
			{
				if (offset < 0x1000)
				{
					std::size_t i;

					const std::size_t max_read_ahead = std::min<std::size_t>(0x12, total_values - offset);

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

			template<typename T>
			inline bool Compress(const unsigned char* const data, const std::size_t data_size, CompressorOutput<T> &output)
			{
				// Produce a series of LZSS compression matches.
				ClownLZSS::Matches matches;
				std::size_t total_matches;
				if (!ClownLZSS::FindOptimalMatches(-1, 0x12, 0x1000, FindExtraMatches, 1 + 8, GetMatchCost, data, 1, data_size, &matches, &total_matches, nullptr))
					return false;

				DescriptorFieldWriter<decltype(output)> descriptor_bits(output);

				// Produce Saxman-formatted data.
				for (ClownLZSS_Match *match = &matches[0]; match != &matches[total_matches]; ++match)
				{
					if (CLOWNLZSS_MATCH_IS_LITERAL(match))
					{
						descriptor_bits.Push(1);
						output.Write(data[match->destination]);
					}
					else
					{
						const std::size_t offset = match->source - 0x12;
						const std::size_t length = match->length;

						descriptor_bits.Push(0);
						output.Write(offset & 0xFF);
						output.Write(((offset & 0xF00) >> 4) | (length - 3));
					}
				}

				return true;
			}

			template<typename T>
			inline bool CompressWithHeader(const unsigned char* const data, const std::size_t data_size, T &&output)
			{
				// Track the location of the header...
				const auto header_position = output.Tell();

				// ...and insert a placeholder there.
				output.WriteLE16(0);

				if (!Compress(data, data_size, output))
					return false;

				// Grab the current position for later.
				const auto end_position = output.Tell();

				// Rewind to the header...
				output.Seek(header_position);

				// ...and complete it.
				output.WriteLE16(end_position - header_position - 2);

				// Seek back to the end of the file just as the caller might expect us to do.
				output.Seek(end_position);

				return true;
			}

			template<typename T>
			inline bool CompressWithoutHeader(const unsigned char* const data, const std::size_t data_size, T &&output)
			{
				return Compress(data, data_size, output);
			}
		}
	}

	template<typename T>
	bool SaxmanCompressWithoutHeader(const unsigned char* const data, const std::size_t data_size, T &&output)
	{
		using namespace Internal;

		CompressorOutput output_wrapped(std::forward<T>(output));
		return Saxman::CompressWithoutHeader(data, data_size, output_wrapped);
	}

	template<typename T>
	bool SaxmanCompressWithHeader(const unsigned char* const data, const std::size_t data_size, T &&output)
	{
		using namespace Internal;

		CompressorOutput output_wrapped(std::forward<T>(output));
		return Saxman::CompressWithHeader(data, data_size, output_wrapped);
	}

	template<typename T>
	bool ModuledSaxmanCompress(const unsigned char* const data, const std::size_t data_size, T &&output, const std::size_t module_size)
	{
		using namespace Internal;

		CompressorOutput output_wrapped(std::forward<T>(output));
		return ModuledCompressionWrapper<2, Endian::Big>(data, data_size, output_wrapped, Saxman::CompressWithHeader, module_size, 2);
	}
}

#endif // CLOWNLZSS_COMPRESSORS_SAXMAN_H
