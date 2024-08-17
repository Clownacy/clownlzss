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

#ifndef CLOWNLZSS_COMPRESSORS_KOSINSKIPLUS_H
#define CLOWNLZSS_COMPRESSORS_KOSINSKIPLUS_H

#include "../clownlzss.h"

#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace KosinskiPlus
		{
			inline constexpr unsigned int TOTAL_DESCRIPTOR_BITS = 8;

			inline std::size_t GetMatchCost(const std::size_t distance, const std::size_t length, [[maybe_unused]] void* const user)
			{
				if (length >= 2 && length <= 5 && distance <= 0x100)
					return 2 + 8 + 2;  // Descriptor bits, offset byte, length bits.
				else if (length >= 3 && length <= 9)
					return 2 + 16;     // Descriptor bits, offset/length bytes.
				else if (length >= 10)
					return 2 + 16 + 8; // Descriptor bits, offset bytes, length byte.
				else
					return 0;          // In the event a match cannot be compressed.
			}

			template<typename T>
			bool Compress(const unsigned char* const data, const std::size_t data_size, T &&output)
			{
				ClownLZSS_Match *matches;
				std::size_t total_matches;

				// Produce a series of LZSS compression matches.
				if (!ClownLZSS_Compress(0x100 + 8, 0x2000, NULL, 1 + 8, GetMatchCost, data, 1, data_size, &matches, &total_matches, nullptr))
					return false;

				// Set up the state.
				typename std::remove_cvref_t<T>::pos_type descriptor_position;
				unsigned int descriptor = 0;
				unsigned int descriptor_bits_remaining;

				const auto BeginDescriptorField = [&]()
				{
					descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

					// Log the placement of the descriptor field.
					descriptor_position = output.Tell();

					// Insert a placeholder.
					output.Write(0);
				};

				const auto FinishDescriptorField = [&]()
				{
					// Back up current position.
					const std::size_t current_position = output.Tell();

					// Go back to the descriptor field.
					output.Seek(descriptor_position);

					// Write the complete descriptor field.
					output.Write(descriptor & 0xFF);

					// Seek back to where we were before.
					output.Seek(current_position);
				};

				const auto PutDescriptorBit = [&](const bool bit)
				{
					if (descriptor_bits_remaining == 0)
					{
						FinishDescriptorField();
						BeginDescriptorField();
					}

					--descriptor_bits_remaining;

					descriptor <<= 1;

					descriptor |= bit;
				};

				// Begin first descriptor field.
				BeginDescriptorField();

				// Produce Kosinski+-formatted data.
				for (ClownLZSS_Match *match = matches; match != &matches[total_matches]; ++match)
				{
					if (CLOWNLZSS_MATCH_IS_LITERAL(match))
					{
						PutDescriptorBit(1);
						output.Write(data[match->destination]);
					}
					else
					{
						const std::size_t distance = match->destination - match->source;
						const std::size_t length = match->length;

						if (length >= 2 && length <= 5 && distance <= 0x100)
						{
							PutDescriptorBit(0);
							PutDescriptorBit(0);
							output.Write(-distance & 0xFF);
							PutDescriptorBit(!!((length - 2) & 2));
							PutDescriptorBit(!!((length - 2) & 1));
						}
						else if (length >= 3 && length <= 9)
						{
							PutDescriptorBit(0);
							PutDescriptorBit(1);
							output.Write(((-distance >> (8 - 3)) & 0xF8) | ((10 - length) & 7));
							output.Write(-distance & 0xFF);
						}
						else //if (length >= 10)
						{
							PutDescriptorBit(0);
							PutDescriptorBit(1);
							output.Write((-distance >> (8 - 3)) & 0xF8);
							output.Write(-distance & 0xFF);
							output.Write(length - 9);
						}
					}
				}

				// We don't need the matches anymore.
				free(matches);

				// Add the terminator match.
				PutDescriptorBit(0);
				PutDescriptorBit(1);
				output.Write(0xF0);
				output.Write(0x00);
				output.Write(0x00);

				// The descriptor field may be incomplete, so move the bits into their proper place.
				descriptor <<= descriptor_bits_remaining;

				// Finish last descriptor field.
				FinishDescriptorField();

				return true;
			}
		}
	}

	template<typename T>
	bool KosinskiPlusCompress(const unsigned char* const data, const std::size_t data_size, T &&output)
	{
		using namespace Internal;

		return KosinskiPlus::Compress(data, data_size, CompressorOutput(output));
	}

	template<typename T>
	bool ModuledKosinskiPlusCompress(const unsigned char* const data, const std::size_t data_size, T &&output, const size_t module_size, const size_t module_alignment)
	{
		return Internal::ModuledCompressionWrapper(data, data_size, CompressorOutput(output), KosinskiPlusCompress, module_size, module_alignment);
	}
}

#endif // CLOWNLZSS_COMPRESSORS_KOSINSKIPLUS_H
