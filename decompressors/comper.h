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
					output.Write(input.Read());
					output.Write(input.Read());
				}
				else
				{
					// Dictionary match.
					const unsigned int raw_offset = input.Read();
					const unsigned int offset = (0x100 - raw_offset) * 2;
					const unsigned int raw_count = input.Read();
					const unsigned int count = (raw_count + 1) * 2;

					if (raw_count == 0)
						break;

					output.template Copy<(0xFF + 1) * 2>(offset, count);
				}
			}
		}
	}

	template<typename T1, typename T2>
	void ComperDecompress(T1 &&input, T2 &&output)
	{
		Internal::ComperDecompress(Input(input), Output(output));
	}
}

#endif // CLOWNLZSS_DECOMPRESSORS_COMPER_H
