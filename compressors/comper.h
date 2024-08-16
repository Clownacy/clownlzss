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

#ifndef CLOWNLZSS_COMPRESSORS_COMPER_H
#define CLOWNLZSS_COMPRESSORS_COMPER_H

#include "clownlzss.h"

#include "../decompressors/common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace Comper
		{
			inline constexpr unsigned int TOTAL_DESCRIPTOR_BITS = 16;

			inline std::size_t GetMatchCost([[maybe_unused]] const std::size_t distance, [[maybe_unused]] const std::size_t length, [[maybe_unused]] void* const user)
			{
				return 1 + 16;	// Descriptor bit, offset/length bytes.
			}

			template<typename T>
			bool Compress(const unsigned char* const data, const std::size_t data_size, T &&output)
			{
				constexpr unsigned int bytes_per_value = 2;

				// Cannot compress data that is an odd number of bytes long.
				if (data_size % bytes_per_value != 0)
					return false;

				ClownLZSS_Match *matches;
				std::size_t total_matches;

				// Produce a series of LZSS compression matches.
				if (!ClownLZSS_Compress(0x100, 0x100, NULL, 1 + 16, GetMatchCost, data, bytes_per_value, data_size / bytes_per_value, &matches, &total_matches, nullptr))
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
					output.Write(0);
				};

				const auto FinishDescriptorField = [&]()
				{
					// Back up current position.
					const auto current_position = output.Tell();

					// Go back to the descriptor field.
					output.Seek(descriptor_position);

					// Write the complete descriptor field.
					output.Write((descriptor >> (8 * 1)) & 0xFF);
					output.Write((descriptor >> (8 * 0)) & 0xFF);

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

				// Produce Comper-formatted data.
				for (ClownLZSS_Match *match = matches; match != &matches[total_matches]; ++match)
				{
					if (CLOWNLZSS_MATCH_IS_LITERAL(match))
					{
						PutDescriptorBit(0);
						output.Write(data[match->destination * 2 + 0]);
						output.Write(data[match->destination * 2 + 1]);
					}
					else
					{
						const std::size_t distance = match->destination - match->source;
						const std::size_t length = match->length;

						PutDescriptorBit(1);
						output.Write(-distance & 0xFF);
						output.Write(length - 1);
					}
				}

				// We don't need the matches anymore.
				free(matches);

				// Add the terminator match.
				PutDescriptorBit(1);
				output.Write(0);
				output.Write(0);

				// The descriptor field may be incomplete, so move the bits into their proper place.
				descriptor <<= descriptor_bits_remaining;

				// Finish last descriptor field.
				FinishDescriptorField();

				return true;
			}
		}
	}

	template<typename T>
	bool ComperCompress(const unsigned char* const data, const std::size_t data_size, T &&output)
	{
		using namespace Internal;

		return Comper::Compress(data, data_size, CompressorOutput(output));
	}
}

#endif // CLOWNLZSS_COMPRESSORS_COMPER_H
