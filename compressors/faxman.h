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

#ifndef CLOWNLZSS_COMPRESSORS_FAXMAN_H
#define CLOWNLZSS_COMPRESSORS_FAXMAN_H

#include <algorithm>
#include <utility>

#include "../bitfield.h"
#include "clownlzss.h"
#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace Faxman
		{
			template<typename T>
			using DescriptorFieldWriter = BitField::DescriptorFieldWriter<1, BitField::WriteWhen::BeforePush, BitField::PushWhere::High, BitField::Endian::Little, T>;

			inline std::size_t GetMatchCost(const std::size_t distance, const std::size_t length, [[maybe_unused]] void* const user)
			{
				if (length >= 2 && length <= 5 && distance <= 0x100)
					return 2 + 8 + 2;
				else if (length >= 3)
					return 2 + 16; // Descriptor bit, offset/length bits.
				else
					return 0;
			}

			inline void FindExtraMatches(const unsigned char* const data, const std::size_t data_size, const std::size_t offset, ClownLZSS_GraphEdge* const node_meta_array, [[maybe_unused]] void* const user)
			{
				if (offset < 0x800)
				{
					std::size_t k;

					const std::size_t max_read_ahead = std::min<std::size_t>(0x1F + 3, data_size - offset);

					for (k = 0; k < max_read_ahead; ++k)
					{
						if (data[offset + k] == 0)
						{
							const unsigned int cost = (k + 1 >= 3) ? 2 + 16 : 0;

							if (cost != 0 && node_meta_array[offset + k + 1].u.cost > node_meta_array[offset].u.cost + cost)
							{
								node_meta_array[offset + k + 1].u.cost = node_meta_array[offset].u.cost + cost;
								node_meta_array[offset + k + 1].previous_node_index = offset;
								node_meta_array[offset + k + 1].match_offset = offset;
							}
						}
						else
						{
							break;
						}
					}
				}
			}

			template<typename T>
			bool Compress(const unsigned char* const data, const std::size_t data_size, CompressorOutput<T> &output)
			{
				// Produce a series of LZSS compression matches.
				ClownLZSS::Matches matches;
				std::size_t total_matches;
				if (!ClownLZSS::FindOptimalMatches(-1, 0x1F + 3, 0x800, FindExtraMatches, 1 + 8, GetMatchCost, data, 1, data_size, &matches, &total_matches, nullptr))
					return false;

				// Track the location of the header...
				const auto header_position = output.Tell();

				// ...and insert a placeholder there.
				output.WriteLE16(0);

				DescriptorFieldWriter<decltype(output)> descriptor_bits(output);
				unsigned int descriptor_bits_total = 0;

				const auto PushDescriptorBit = [&](const bool value)
				{
					descriptor_bits.Push(value);
					++descriptor_bits_total;
				};

				// Produce Faxman-formatted data.
				for (ClownLZSS_Match *match = &matches[0]; match != &matches[total_matches]; ++match)
				{
					if (CLOWNLZSS_MATCH_IS_LITERAL(match))
					{
						PushDescriptorBit(1);
						output.Write(data[match->destination]);
					}
					else
					{
						const std::size_t distance = match->destination == match->source ? 0x800 : match->destination - match->source;
						const std::size_t length = match->length;

						if (length >= 2 && length <= 5 && distance <= 0x100)
						{
							PushDescriptorBit(0);
							PushDescriptorBit(0);
							output.Write(-distance & 0xFF);
							PushDescriptorBit(!!((length - 2) & 2));
							PushDescriptorBit(!!((length - 2) & 1));
						}
						else //if (length >= 3)
						{
							PushDescriptorBit(0);
							PushDescriptorBit(1);
							output.Write((distance - 1) & 0xFF);
							output.Write((((distance - 1) & 0x700) >> 3) | (length - 3));
						}
					}
				}

				// Grab the current position for later.
				const auto end_position = output.Tell();

				// Rewind to the header...
				output.Seek(header_position);

				// ...and complete it.
				output.WriteLE16(descriptor_bits_total);

				// Seek back to the end of the file just as the caller might expect us to do.
				output.Seek(end_position);

				return true;
			}
		}
	}

	template<typename T>
	bool FaxmanCompress(const unsigned char* const data, const std::size_t data_size, T &&output)
	{
		using namespace Internal;

		CompressorOutput output_wrapped(std::forward<T>(output));
		return Faxman::Compress(data, data_size, output_wrapped);
	}

	template<typename T>
	bool ModuledFaxmanCompress(const unsigned char* const data, const std::size_t data_size, T &&output, const std::size_t module_size)
	{
		using namespace Internal;

		CompressorOutput output_wrapped(std::forward<T>(output));
		return ModuledCompressionWrapper<2, Endian::Big>(data, data_size, output_wrapped, Faxman::Compress, module_size, 2);
	}
}

#endif // CLOWNLZSS_COMPRESSORS_FAXMAN_H
