#ifndef CLOWNLZSS_DECOMPRESSORS_SAXMAN_H
#define CLOWNLZSS_DECOMPRESSORS_SAXMAN_H

#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		template<typename T1, typename T2>
		void SaxmanDecompress(T1 &&input, T2 &&output, const unsigned int compressed_length)
		{
			unsigned int input_position = 0;
			unsigned int output_position = 0;
			unsigned int descriptor_bits_remaining = 0;
			unsigned char descriptor_bits;

			while (input_position < compressed_length)
			{
				if (descriptor_bits_remaining == 0)
				{
					descriptor_bits_remaining = 8;
					descriptor_bits = input.Read();
					++input_position;
				}

				if ((descriptor_bits & 1) != 0)
				{
					// Uncompressed.
					output.Write(input.Read());
					++input_position;
					++output_position;
				}
				else
				{
					// Dictionary match.
					const unsigned int first_byte = input.Read();
					const unsigned int second_byte = input.Read();
					const unsigned int dictionary_index = (first_byte | ((second_byte << 4) & 0xF00)) + (0xF + 3);
					const unsigned int count = (second_byte & 0xF) + 3;
					const unsigned int offset = (output_position - dictionary_index) & 0xFFF;

					if (offset > output_position)
					{
						// Zero-fill.
						output.Fill(0, count);
					}
					else
					{
						// Copy.
						output.template Copy<0xF + 3>(offset, count);
					}

					input_position += 2;
					output_position += count;
				}

				descriptor_bits >>= 1;
				--descriptor_bits_remaining;
			}
		}
	}

	template<typename T1, typename T2>
	void SaxmanDecompress(T1 &&input, T2 &&output, const unsigned int compressed_length)
	{
		Internal::SaxmanDecompress(Input(input), Output(output), compressed_length);
	}
}

#endif // CLOWNLZSS_DECOMPRESSORS_SAXMAN_H
