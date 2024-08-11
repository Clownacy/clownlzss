#ifndef CLOWNLZSS_DECOMPRESSORS_KOSINSKIPLUS_H
#define CLOWNLZSS_DECOMPRESSORS_KOSINSKIPLUS_H

#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		template<typename T1, typename T2>
		inline void KosinskiPlusDecompress(T1 &&input, T2 &&output)
		{
			BitField<1, ReadWhen::BeforePop, PopWhere::High, Endian::Big, T1> descriptor_bits(input);

			for (;;)
			{
				if (descriptor_bits.Pop())
				{
					output.Write(Read(input));
				}
				else
				{
					unsigned int offset;
					unsigned int count;

					if (descriptor_bits.Pop())
					{
						const unsigned int high_byte = Read(input);
						const unsigned int low_byte = Read(input);

						offset = ((high_byte & 0xF8) << 5) | low_byte;
						offset = 0x2000 - offset;
						count = high_byte & 7;

						if (count != 0)
						{
							count = 10 - count;
						}
						else
						{
							count = Read(input) + 9;

							if (count == 9)
								break;
						}
					}
					else
					{
						offset = 0x100 - Read(input);

						count = 2;

						if (descriptor_bits.Pop())
							count += 2;
						if (descriptor_bits.Pop())
							count += 1;
					}

					output.template Copy<0x100>(offset, count);
				}
			}
		}
	}

	template<std::input_iterator T1, Internal::random_access_input_output_iterator T2>
	inline void KosinskiPlusDecompress(T1 input, T2 output)
	{
		Internal::KosinskiPlusDecompress(input, Output(output));
	}

	#if __STDC_HOSTED__ == 1
	inline void KosinskiPlusDecompress(std::istream &input, std::iostream &output)
	{
		Internal::KosinskiPlusDecompress(input, Output(output));
	}
	#endif
}

#endif // CLOWNLZSS_DECOMPRESSORS_KOSINSKI_H
