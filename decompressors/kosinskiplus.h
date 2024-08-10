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
			unsigned int descriptor_bits_remaining;
			unsigned char descriptor_bits;

			const auto GetDescriptor = [&]()
			{
				descriptor_bits = Read(input);
				descriptor_bits_remaining = 8;

			};

			const auto PopDescriptor = [&]()
			{
				if (descriptor_bits_remaining == 0)
					GetDescriptor();

				const bool result = (descriptor_bits & 0x80) != 0;

				descriptor_bits <<= 1;
				--descriptor_bits_remaining;

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

						if (PopDescriptor())
							count += 2;
						if (PopDescriptor())
							count += 1;
					}

					Copy<0x100>(output, offset, count);
				}
			}
		}
	}

	template<std::input_iterator T1, Internal::random_access_input_output_iterator T2>
	inline void KosinskiPlusDecompress(T1 input, T2 output)
	{
		Internal::KosinskiPlusDecompress(input, output);
	}

	#if __STDC_HOSTED__ == 1
	inline void KosinskiPlusDecompress(std::istream &input, std::iostream &output)
	{
		Internal::KosinskiPlusDecompress(input, output);
	}
	#endif
}

#endif /* CLOWNLZSS_DECOMPRESSORS_KOSINSKI_H */
