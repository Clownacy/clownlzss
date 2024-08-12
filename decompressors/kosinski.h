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
					output.Write(input.Read());
				}
				else
				{
					unsigned int offset;
					unsigned int count;

					if (descriptor_bits.Pop())
					{
						const unsigned int low_byte = input.Read();
						const unsigned int high_byte = input.Read();

						offset = ((high_byte & 0xF8) << 5) | low_byte;
						offset = 0x2000 - offset;
						count = high_byte & 7;

						if (count != 0)
						{
							count += 2;
						}
						else
						{
							count = input.Read() + 1;

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

						offset = 0x100 - input.Read();
					}

					output.template Copy<0x100>(offset, count);
				}
			}
		}
	}

	template<typename T1, typename T2>
	void KosinskiDecompress(T1 &&input, T2 &&output)
	{
		Internal::KosinskiDecompress(Input(input), Output(output));
	}
}

#endif // CLOWNLZSS_DECOMPRESSORS_KOSINSKI_H
