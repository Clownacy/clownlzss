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

#ifndef CLOWNLZSS_COMPRESSORS_CHAMELEON_H
#define CLOWNLZSS_COMPRESSORS_CHAMELEON_H

#include <utility>

#include "../bitfield.h"
#include "clownlzss.h"
#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace Chameleon
		{
			template<typename T>
			using BitFieldWriter = BitField::Writer<1, BitField::WriteWhen::BeforePush, BitField::PushWhere::Low, BitField::Endian::Big, T>;

			inline std::size_t GetMatchCost(const std::size_t distance, const std::size_t length, [[maybe_unused]] void* const user)
			{
				if (length >= 2 && length <= 3 && distance < 0x100)
					return 2 + 8 + 1;         /* Descriptor bits, offset byte, length bit */
				else if (length >= 3 && length <= 5)
					return 2 + 3 + 8 + 2;     /* Descriptor bits, offset bits, offset byte, length bits */
				else if (length >= 6)
					return 2 + 3 + 8 + 2 + 8; /* Descriptor bits, offset bits, offset byte, (blank) length bits, length byte */
				else
					return 0;                 /* In the event a match cannot be compressed */
			}

			template<typename T>
			bool Compress(const unsigned char* const data, const std::size_t data_size, CompressorOutput<T> &output)
			{
				/* Produce a series of LZSS compression matches. */
				/* Yes, the first two values really are lower than usual by 1. */
				ClownLZSS::Matches matches;
				std::size_t total_matches;
				if (!ClownLZSS::FindOptimalMatches(-1, 0xFF, 0x7FF, nullptr, 1 + 8, GetMatchCost, data, 1, data_size, &matches, &total_matches, nullptr))
					return false;

				/* Track the location of the header... */
				const auto header_position = output.Tell();

				/* ...and insert a placeholder there. */
				output.WriteBE16(0);

				{
					BitFieldWriter<decltype(output)> descriptor_bits(output);

					/* Produce Chameleon-formatted data. */
					/* Unlike many other LZSS formats, Chameleon stores the descriptor fields separately from the rest of the data. */
					/* Iterate over the compression matches, outputting just the descriptor fields. */
					for (ClownLZSS_Match *match = &matches[0]; match != &matches[total_matches]; ++match)
					{
						if (CLOWNLZSS_MATCH_IS_LITERAL(match))
						{
							descriptor_bits.Push(1);
						}
						else
						{
							const std::size_t distance = match->destination - match->source;
							const std::size_t length = match->length;

							if (length >= 2 && length <= 3 && distance < 0x100)
							{
								descriptor_bits.Push(0);
								descriptor_bits.Push(0);
								descriptor_bits.Push(length == 3);
							}
							else if (length >= 3 && length <= 5)
							{
								descriptor_bits.Push(0);
								descriptor_bits.Push(1);
								descriptor_bits.Push(!!(distance & (1 << 10)));
								descriptor_bits.Push(!!(distance & (1 << 9)));
								descriptor_bits.Push(!!(distance & (1 << 8)));
								descriptor_bits.Push(length == 5);
								descriptor_bits.Push(length == 4);
							}
							else /*if (length >= 6)*/
							{
								descriptor_bits.Push(0);
								descriptor_bits.Push(1);
								descriptor_bits.Push(!!(distance & (1 << 10)));
								descriptor_bits.Push(!!(distance & (1 << 9)));
								descriptor_bits.Push(!!(distance & (1 << 8)));
								descriptor_bits.Push(1);
								descriptor_bits.Push(1);
							}
						}
					}

					/* Add the terminator match. */
					descriptor_bits.Push(0);
					descriptor_bits.Push(1);
					descriptor_bits.Push(0);
					descriptor_bits.Push(0);
					descriptor_bits.Push(0);
					descriptor_bits.Push(1);
					descriptor_bits.Push(1);
				}

				/* Chameleon's header contains the size of the descriptor fields, so, now that we know that, let's fill it in. */
				const auto current_position = output.Tell();
				output.Seek(header_position);
				output.WriteBE16(current_position - header_position - 2);
				output.Seek(current_position);

				/* Iterate over the compression matches again, now outputting just the literals and offset/length pairs. */
				for (ClownLZSS_Match *match = &matches[0]; match != &matches[total_matches]; ++match)
				{
					if (CLOWNLZSS_MATCH_IS_LITERAL(match))
					{
						output.Write(data[match->destination]);
					}
					else
					{
						const std::size_t distance = match->destination - match->source;
						const std::size_t length = match->length;

						if (length >= 2 && length <= 3 && distance < 0x100)
						{
							output.Write(distance);
						}
						else if (length >= 3 && length <= 5)
						{
							output.Write(distance & 0xFF);
						}
						else /*if (length >= 6)*/
						{
							output.Write(distance & 0xFF);
							output.Write(length);
						}
					}
				}

				/* Add the terminator match. */
				output.Write(0);
				output.Write(0);

				return true;
			}
		}
	}

	template<typename T>
	bool ChameleonCompress(const unsigned char* const data, const std::size_t data_size, T &&output)
	{
		using namespace Internal;

		CompressorOutput output_wrapped(std::forward<T>(output));
		return Chameleon::Compress(data, data_size, output_wrapped);
	}

	template<typename T>
	bool ModuledChameleonCompress(const unsigned char* const data, const std::size_t data_size, T &&output, const std::size_t module_size)
	{
		using namespace Internal;

		CompressorOutput output_wrapped(std::forward<T>(output));
		return ModuledCompressionWrapper<2, Endian::Big>(data, data_size, output_wrapped, Chameleon::Compress, module_size, 2);
	}
}

#endif // CLOWNLZSS_COMPRESSORS_CHAMELEON_H
