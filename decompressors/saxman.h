#ifndef CLOWNLZSS_DECOMPRESSORS_SAXMAN_H
#define CLOWNLZSS_DECOMPRESSORS_SAXMAN_H

#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		template<typename T1, typename T2>
		void SaxmanDecompress(T1 &&input, T2 &&output)
		{
			BitField<1, ReadWhen::BeforePop, PopWhere::Low, Endian::Little, T1> descriptor_bits(input);

			unsigned int output_position = 0;

			while (!input.AtEnd())
			{
				if (descriptor_bits.Pop())
				{
					// Uncompressed.
					output.Write(input.Read());
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

					output_position += count;
				}
			}
		}
	}

	template<typename T1, typename T2>
	void SaxmanDecompress(T1 &&input, T2 &&output, const unsigned int compressed_length)
	{
		Internal::SaxmanDecompress(InputWithLength(input, compressed_length), Output(output));
	}

	template<std::random_access_iterator T1, std::random_access_iterator T2>
	void SaxmanDecompress(T1 input, T1 input_end, T2 output)
	{
		SaxmanDecompress(input, output, input_end - input);
	}
}

#endif // CLOWNLZSS_DECOMPRESSORS_SAXMAN_H
