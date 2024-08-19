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

#ifndef CLOWNLZSS_DECOMPRESSORS_FAXMAN_H
#define CLOWNLZSS_DECOMPRESSORS_FAXMAN_H

#include "bitfield.h"
#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace Faxman
		{
			template<typename T>
			using DecompressorOutput = DecompressorOutput<T, 0x800, 0x1F + 3>;

			template<typename T>
			using BitField = BitField<1, ReadWhen::BeforePop, PopWhere::Low, Endian::Little, T>;

			template<typename T1, typename T2>
			void Decompress(DecompressorInput<T1> input, DecompressorOutput<T2> output)
			{
				unsigned int descriptor_bits_remaining = input.ReadLE16();

				BitField descriptor_bits(input);

				const auto PopDescriptorBit = [&]()
				{
					--descriptor_bits_remaining;
					return descriptor_bits.Pop();
				};

				unsigned int output_position = 0;

				while (descriptor_bits_remaining != 0)
				{
					if (PopDescriptorBit())
					{
						// Uncompressed.
						output.Write(input.Read());
						++output_position;
					}
					else
					{
						// Dictionary match.
						unsigned int distance, count;

						if (PopDescriptorBit())
						{
							const unsigned int first_byte = input.Read();
							const unsigned int second_byte = input.Read();

							distance = (first_byte | ((second_byte << 3) & 0x700)) + 1;
							count = (second_byte & 0x1F) + 3;
						}
						else
						{
							distance = 0x100 - input.Read();
							count = 2;

							if (PopDescriptorBit())
								count += 2;
							if (PopDescriptorBit())
								count += 1;
						}

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

						output_position += count;
					}
				}
			}
		}
	}

	template<typename T1, typename T2>
	void FaxmanDecompress(T1 &&input, T2 &&output)
	{
		using namespace Internal;

		Faxman::Decompress(DecompressorInput(input), Faxman::DecompressorOutput(output));
	}

	template<std::random_access_iterator T1, std::random_access_iterator T2>
	void FaxmanDecompress(T1 input, T1 input_end, T2 output)
	{
		FaxmanDecompress(input, output, input_end - input);
	}
}

#endif // CLOWNLZSS_DECOMPRESSORS_FAXMAN_H
