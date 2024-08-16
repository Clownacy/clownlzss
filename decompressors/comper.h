#ifndef CLOWNLZSS_DECOMPRESSORS_COMPER_H
#define CLOWNLZSS_DECOMPRESSORS_COMPER_H

#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace Comper
		{
			constexpr unsigned int RawDistanceToDistance(const unsigned int raw_distance)
			{
				return (0x100 - raw_distance) * 2;
			}

			constexpr unsigned int RawCountToCount(const unsigned int raw_count)
			{
				return (raw_count + 1) * 2;
			}

			template<typename T>
			using DecompressorOutput = DecompressorOutput<T, RawDistanceToDistance(0), RawCountToCount(0xFF)>;

			template<typename T1, typename T2>
			void Decompress(T1 &&input, T2 &&output)
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
						const unsigned int raw_distance = input.Read();
						const unsigned int distance = RawDistanceToDistance(raw_distance);
						const unsigned int raw_count = input.Read();
						const unsigned int count = RawCountToCount(raw_count);

						if (raw_count == 0)
							break;

						output.Copy(distance, count);
					}
				}
			}
		}
	}

	template<typename T1, typename T2>
	void ComperDecompress(T1 &&input, T2 &&output)
	{
		using namespace Internal;

		Comper::Decompress(DecompressorInput(input), Comper::DecompressorOutput(output));
	}
}

#endif // CLOWNLZSS_DECOMPRESSORS_COMPER_H
