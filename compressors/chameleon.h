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

#ifndef CLOWNLZSS_COMPRESSORS_CHAMELEON_H
#define CLOWNLZSS_COMPRESSORS_CHAMELEON_H

#include "clownlzss.h"
#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace Chameleon
		{
			inline constexpr unsigned int TOTAL_DESCRIPTOR_BITS = 8;

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
			bool Compress(const unsigned char* const data, const std::size_t data_size, T &&output)
			{
				/* Produce a series of LZSS compression matches. */
				/* Yes, the first two values really are lower than usual by 1. */
				ClownLZSS::Matches matches;
				std::size_t total_matches;
				if (!ClownLZSS::FindOptimalMatches(0xFF, 0x7FF, nullptr, 1 + 8, GetMatchCost, data, 1, data_size, &matches, &total_matches, nullptr))
					return false;

				/* Set up the state. */
				typename std::remove_cvref_t<T>::pos_type descriptor_position;
				unsigned int descriptor = 0;
				unsigned int descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

				const auto PutDescriptorBit = [&](const bool bit)
				{
					if (descriptor_bits_remaining == 0)
					{
						descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

						output.Write(descriptor & 0xFF);
					}

					--descriptor_bits_remaining;

					descriptor <<= 1;

					descriptor |= bit;
				};

				/* Track the location of the header... */
				const auto header_position = output.Tell();

				/* ...and insert a placeholder there. */
				output.Write(0);
				output.Write(0);

				/* Produce Chameleon-formatted data. */
				/* Unlike many other LZSS formats, Chameleon stores the descriptor fields separately from the rest of the data. */
				/* Iterate over the compression matches, outputting just the descriptor fields. */
				for (ClownLZSS_Match *match = &matches[0]; match != &matches[total_matches]; ++match)
				{
					if (CLOWNLZSS_MATCH_IS_LITERAL(match))
					{
						PutDescriptorBit(1);
					}
					else
					{
						const std::size_t distance = match->destination - match->source;
						const std::size_t length = match->length;

						if (length >= 2 && length <= 3 && distance < 0x100)
						{
							PutDescriptorBit(0);
							PutDescriptorBit(0);
							PutDescriptorBit(length == 3);
						}
						else if (length >= 3 && length <= 5)
						{
							PutDescriptorBit(0);
							PutDescriptorBit(1);
							PutDescriptorBit(!!(distance & (1 << 10)));
							PutDescriptorBit(!!(distance & (1 << 9)));
							PutDescriptorBit(!!(distance & (1 << 8)));
							PutDescriptorBit(length == 5);
							PutDescriptorBit(length == 4);
						}
						else /*if (length >= 6)*/
						{
							PutDescriptorBit(0);
							PutDescriptorBit(1);
							PutDescriptorBit(!!(distance & (1 << 10)));
							PutDescriptorBit(!!(distance & (1 << 9)));
							PutDescriptorBit(!!(distance & (1 << 8)));
							PutDescriptorBit(1);
							PutDescriptorBit(1);
						}
					}
				}

				/* Add the terminator match. */
				PutDescriptorBit(0);
				PutDescriptorBit(1);
				PutDescriptorBit(0);
				PutDescriptorBit(0);
				PutDescriptorBit(0);
				PutDescriptorBit(1);
				PutDescriptorBit(1);

				/* The descriptor field may be incomplete, so move the bits into their proper place. */
				descriptor <<= descriptor_bits_remaining;

				/* Write last descriptor field. */
				output.Write(descriptor);

				/* Chameleon's header contains the size of the descriptor fields, so, now that we know that, let's fill it in. */
				const auto current_position = output.Tell();
				output.Seek(header_position);
				output.Write(((current_position - header_position - 2) >> (8 * 1)) & 0xFF);
				output.Write(((current_position - header_position - 2) >> (8 * 0)) & 0xFF);
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

		return Chameleon::Compress(data, data_size, CompressorOutput(output));
	}

	template<typename T>
	bool ModuledChameleonCompress(const unsigned char* const data, const std::size_t data_size, T &&output, const std::size_t module_size, const std::size_t module_alignment)
	{
		using namespace Internal;

		return ModuledCompressionWrapper(data, data_size, CompressorOutput(output), Chameleon::Compress, module_size, module_alignment);
	}
}

#endif // CLOWNLZSS_COMPRESSORS_CHAMELEON_H
