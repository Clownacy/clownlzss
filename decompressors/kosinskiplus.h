#ifndef CLOWNLZSS_DECOMPRESSORS_KOSINSKIPLUS_H
#define CLOWNLZSS_DECOMPRESSORS_KOSINSKIPLUS_H

#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace KosinskiPlus
		{
			template<typename T>
			using DecompressorOutput = DecompressorOutput<T, 0x2000, 0x100>;

			template<typename T1, typename T2>
			void Decompress(T1 &&input, T2 &&output)
			{
				BitField<1, ReadWhen::BeforePop, PopWhere::High, Endian::Big, T1> descriptor_bits(input);

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
							const unsigned int high_byte = input.Read();
							const unsigned int low_byte = input.Read();

							offset = ((high_byte & 0xF8) << 5) | low_byte;
							offset = 0x2000 - offset;
							count = high_byte & 7;

							if (count != 0)
							{
								count = 10 - count;
							}
							else
							{
								count = input.Read() + 9;

								if (count == 9)
									break;
							}
						}
						else
						{
							offset = 0x100 - input.Read();

							count = 2;

							if (descriptor_bits.Pop())
								count += 2;
							if (descriptor_bits.Pop())
								count += 1;
						}

						output.Copy(offset, count);
					}
				}
			}
		}
	}

	template<typename T1, typename T2>
	void KosinskiPlusDecompress(T1 &&input, T2 &&output)
	{
		using namespace Internal;

		KosinskiPlus::Decompress(DecompressorInput(input), KosinskiPlus::DecompressorOutput(output));
	}
}

#endif // CLOWNLZSS_DECOMPRESSORS_KOSINSKI_H
