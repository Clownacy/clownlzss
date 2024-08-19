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

#ifndef CLOWNLZSS_DECOMPRESSORS_SAXMAN_H
#define CLOWNLZSS_DECOMPRESSORS_SAXMAN_H

#include "bitfield.h"
#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace Saxman
		{
			template<typename T>
			using DecompressorOutput = DecompressorOutput<T, 0x1000, 0xF + 3>;

			template<typename T>
			using BitField = BitField<1, ReadWhen::BeforePop, PopWhere::Low, Endian::Little, T>;

			template<typename T1, typename T2>
			void Decompress(T1 &&input, T2 &&output, const unsigned int compressed_length)
			{
				const auto input_start_position = input.Tell();
				const auto output_start_position = output.Tell();

				BitField descriptor_bits(input);

				while (input.Distance(input_start_position) < compressed_length)
				{
					if (descriptor_bits.Pop())
					{
						// Uncompressed.
						output.Write(input.Read());
					}
					else
					{
						// Dictionary match.
						const unsigned int first_byte = input.Read();
						const unsigned int second_byte = input.Read();
						const unsigned int dictionary_index = (first_byte | ((second_byte << 4) & 0xF00)) + (0xF + 3);
						const unsigned int count = (second_byte & 0xF) + 3;
						const unsigned int output_position = output.Distance(output_start_position);
						const unsigned int distance = (output_position - dictionary_index) % 0x1000;

						if (distance > output_position)
						{
							// Zero-fill.
							output.Fill(0, count);
						}
						else
						{
							// Copy.
							output.Copy(distance, count);
						}
					}
				}
			}
		}
	}

	template<typename T1, typename T2>
	void SaxmanDecompress(T1 &&input, T2 &&output, const unsigned int compressed_length)
	{
		using namespace Internal;

		Saxman::Decompress(DecompressorInput(input), Saxman::DecompressorOutput(output), compressed_length);
	}

	template<std::random_access_iterator T1, std::random_access_iterator T2>
	void SaxmanDecompress(T1 input, T1 input_end, T2 output)
	{
		SaxmanDecompress(input, output, input_end - input);
	}
}

#endif // CLOWNLZSS_DECOMPRESSORS_SAXMAN_H
