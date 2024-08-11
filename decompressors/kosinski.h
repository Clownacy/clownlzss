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
			BitField<2, ReadWhen::AfterPop, PopWhere::Low, Endian::Little, T1> descriptor_bits(input);

			for (;;)
			{
				if (descriptor_bits.Pop())
				{
					Write(output, Read(input));
				}
				else
				{
					unsigned int offset;
					unsigned int count;

					if (descriptor_bits.Pop())
					{
						const unsigned int low_byte = Read(input);
						const unsigned int high_byte = Read(input);

						offset = ((high_byte & 0xF8) << 5) | low_byte;
						offset = 0x2000 - offset;
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

						if (descriptor_bits.Pop())
							count += 2;
						if (descriptor_bits.Pop())
							count += 1;

						offset = 0x100 - Read(input);
					}

					Copy<0x100>(output, offset, count);
				}
			}
		}
	}

	template<std::input_iterator T1, Internal::random_access_input_output_iterator T2>
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

#endif // CLOWNLZSS_DECOMPRESSORS_KOSINSKI_H
