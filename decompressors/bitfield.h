#ifndef CLOWNLZSS_BITFIELD_H
#define CLOWNLZSS_BITFIELD_H

namespace ClownLZSS
{
	namespace Internal
	{
		enum class ReadWhen
		{
			BeforePop,
			AfterPop
		};

		enum class PopWhere
		{
			Low,
			High
		};

		enum class Endian
		{
			Big,
			Little
		};

		template<unsigned int total_bytes, ReadWhen read_when, PopWhere pop_where, Endian endian, typename Input>
		class BitField
		{
		private:
			static constexpr unsigned int total_bits = total_bytes * 8;

			Input &input;
			unsigned int bits = 0, bits_remaining;

			void ReadBits()
			{
				bits_remaining = total_bits;

				for (unsigned int i = 0; i < total_bytes; ++i)
				{
					if constexpr(endian == Endian::Big)
					{
						bits <<= 8;
						bits |= input.Read();
					}
					else if constexpr(endian == Endian::Little)
					{
						bits >>= 8;
						bits |= static_cast<decltype(bits)>(input.Read()) << (total_bits - 8);
					}
				}
			};

		public:
			BitField(Input &input)
				: input(input)
			{
				ReadBits();
			}

			bool Pop()
			{
				const auto &CheckReadBits = [&]()
				{
					if (bits_remaining == 0)
						ReadBits();
				};

				if constexpr(read_when == ReadWhen::BeforePop)
					CheckReadBits();

				constexpr unsigned int mask = []() constexpr
				{
					if constexpr(pop_where == PopWhere::High)
						return 1 << (total_bits - 1);
					else if constexpr(pop_where == PopWhere::Low)
						return 1;
				}();

				const bool bit = (bits & mask) != 0;

				if constexpr(pop_where == PopWhere::High)
					bits <<= 1;
				else if constexpr(pop_where == PopWhere::Low)
					bits >>= 1;

				--bits_remaining;

				if constexpr(read_when == ReadWhen::AfterPop)
					CheckReadBits();

				return bit;
			}
		};
	}
}

#endif // CLOWNLZSS_BITFIELD_H
