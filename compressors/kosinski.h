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

#ifndef CLOWNLZSS_COMPRESSORS_KOSINSKI_H
#define CLOWNLZSS_COMPRESSORS_KOSINSKI_H

#include <utility>

#include "../bitfield.h"
#include "clownlzss.h"
#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace Kosinski
		{
			template<typename T>
			using DescriptorFieldWriter = BitField::DescriptorFieldWriter<2, BitField::WriteWhen::AfterPush, BitField::PushWhere::High, BitField::Endian::Little, T>;

			inline std::size_t GetMatchCost(const std::size_t distance, const std::size_t length, [[maybe_unused]] void* const user)
			{
				if (length >= 2 && length <= 5 && distance <= 0x100)
					return 2 + 2 + 8;  // Descriptor bits, length bits, offset byte.
				else if (length >= 3 && length <= 9)
					return 2 + 16;     // Descriptor bits, offset/length bytes.
				else if (length >= 3)
					return 2 + 16 + 8; // Descriptor bits, offset bytes, length byte.
				else
					return 0;          // In the event a match cannot be compressed.
			}

			template<typename T>
			bool Compress(const unsigned char* const data, const std::size_t data_size, CompressorOutput<T> &output)
			{
				// Produce a series of LZSS compression matches.
				ClownLZSS::Matches matches;
				std::size_t total_matches;
				if (!ClownLZSS::FindOptimalMatches(-1, 0x100, 0x2000, nullptr, 1 + 8, GetMatchCost, data, 1, data_size, &matches, &total_matches, nullptr))
					return false;

				DescriptorFieldWriter<decltype(output)> descriptor_bits(output);

				// Produce Kosinski-formatted data.
				for (ClownLZSS_Match *match = &matches[0]; match != &matches[total_matches]; ++match)
				{
					if (CLOWNLZSS_MATCH_IS_LITERAL(match))
					{
						descriptor_bits.Push(1);
						output.Write(data[match->destination]);
					}
					else
					{
						const std::size_t distance = match->destination - match->source;
						const std::size_t length = match->length;

						if (length >= 2 && length <= 5 && distance <= 0x100)
						{
							descriptor_bits.Push(0);
							descriptor_bits.Push(0);
							descriptor_bits.Push(!!((length - 2) & 2));
							descriptor_bits.Push(!!((length - 2) & 1));
							output.Write(-distance & 0xFF);
						}
						else if (length >= 3 && length <= 9)
						{
							descriptor_bits.Push(0);
							descriptor_bits.Push(1);
							output.Write(-distance & 0xFF);
							output.Write(((-distance >> (8 - 3)) & 0xF8) | ((length - 2) & 7));
						}
						else //if (length >= 3)
						{
							descriptor_bits.Push(0);
							descriptor_bits.Push(1);
							output.Write(-distance & 0xFF);
							output.Write((-distance >> (8 - 3)) & 0xF8);
							output.Write(length - 1);
						}
					}
				}

				// Add the terminator match.
				descriptor_bits.Push(0);
				descriptor_bits.Push(1);
				output.Write(0x00);
				output.Write(0xF0);
				output.Write(0x00);

				return true;
			}
		}
	}

	template<typename T>
	bool KosinskiCompress(const unsigned char* const data, const std::size_t data_size, T &&output)
	{
		using namespace Internal;

		CompressorOutput output_wrapped(std::forward<T>(output));
		return Kosinski::Compress(data, data_size, output_wrapped);
	}

	template<typename T>
	bool ModuledKosinskiCompress(const unsigned char* const data, const std::size_t data_size, T &&output, const std::size_t module_size)
	{
		using namespace Internal;

		CompressorOutput output_wrapped(std::forward<T>(output));
		return ModuledCompressionWrapper<2, Endian::Big>(data, data_size, output_wrapped, Kosinski::Compress, module_size, 0x10);
	}
}

#endif // CLOWNLZSS_COMPRESSORS_KOSINSKI_H
