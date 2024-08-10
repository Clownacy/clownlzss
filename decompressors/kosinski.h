#ifndef CLOWNLZSS_DECOMPRESSORS_KOSINSKI_H
#define CLOWNLZSS_DECOMPRESSORS_KOSINSKI_H

#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		template<typename T1, typename T2>
		inline void KosinskiDecompress(T1 &&input, T2 &&output)
		{
			unsigned int descriptor_bits_remaining;
			unsigned short descriptor_bits;

			const auto GetDescriptor = [&]()
			{
				const unsigned int low_byte = Read(input);
				const unsigned int high_byte = Read(input);;

				descriptor_bits = (high_byte << 8) | low_byte;
				descriptor_bits_remaining = 16;

			};

			const auto PopDescriptor = [&]()
			{
				const bool result = (descriptor_bits & 1) != 0;

				descriptor_bits >>= 1;

				if (--descriptor_bits_remaining == 0)
					GetDescriptor();

				return result;
			};

			GetDescriptor();

			for (;;)
			{
				if (PopDescriptor())
				{
					Write(output, Read(input));
				}
				else
				{
					unsigned int offset;
					unsigned int count;

					if (PopDescriptor())
					{
						const unsigned int low_byte = Read(input);
						const unsigned int high_byte = Read(input);

						offset = ((high_byte & 0xF8) << 5) | low_byte;
						offset = 0x2000 - offset; // Convert from negative two's-complement to positive.
						count = high_byte & 7;

						if (count != 0)
						{
							count += 2;
						}
						else
						{
							count = Read(input) + 1;

							if (count == 1)
								break;
							else if (count == 2)
								continue;
						}
					}
					else
					{
						count = 2;

						if (PopDescriptor())
							count += 2;
						if (PopDescriptor())
							count += 1;

						offset = 0x100 - Read(input); // Convert from negative two's-complement to positive.
					}

					Copy<0x100>(output, offset, count);
				}
			}
		}
	}

	template<std::input_or_output_iterator T1, std::input_or_output_iterator T2>
	inline void KosinskiDecompress(T1 input, T2 output)
	{
		Internal::KosinskiDecompress(input, output);
	}

	#if __STDC_HOSTED__ == 1
	inline void KosinskiDecompress(std::istream &input, std::iostream &output)
	{
		Internal::KosinskiDecompress(input, output);
	}
	#endif
}

#endif /* CLOWNLZSS_DECOMPRESSORS_KOSINSKI_H */
