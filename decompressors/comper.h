#ifndef CLOWNLZSS_DECOMPRESSORS_COMPER_H
#define CLOWNLZSS_DECOMPRESSORS_COMPER_H

#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		template<typename T1, typename T2>
		void ComperDecompress(T1 &&input, T2 &&output)
		{
			BitField<2, ReadWhen::BeforePop, PopWhere::High, Endian::Big, T1> descriptor_bits(input);

			for (;;)
			{
				if (!descriptor_bits.Pop())
				{
					// Uncompressed.
					Write(output, Read(input));
					Write(output, Read(input));
				}
				else
				{
					// Dictionary match.
					const unsigned int raw_offset = Read(input);
					const unsigned int offset = (0x100 - raw_offset) * 2;
					const unsigned int raw_count = Read(input);
					const unsigned int count = (raw_count + 1) * 2;

					if (raw_count == 0)
						break;

					Copy<(0xFF + 1) * 2>(output, offset, count);
				}
			}
		}
	}

	template<std::input_iterator T1, Internal::random_access_input_output_iterator T2>
	inline void ComperDecompress(T1 input, T2 output)
	{
		Internal::ComperDecompress(input, output);
	}

	#if __STDC_HOSTED__ == 1
	inline void ComperDecompress(std::istream &input, std::iostream &output)
	{
		Internal::ComperDecompress(input, output);
	}
	#endif
}

#endif // CLOWNLZSS_DECOMPRESSORS_COMPER_H
