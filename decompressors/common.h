#ifndef CLOWNLZSS_DECOMPRESSORS_COMMON_H
#define CLOWNLZSS_DECOMPRESSORS_COMMON_H

#include <algorithm>
#include <array>
#include <iterator>

namespace ClownLZSS
{
	namespace Internal
	{
		template<typename T>
		concept random_access_input_output_iterator = std::random_access_iterator<T> && std::input_iterator<T> && std::output_iterator<T, unsigned char>;
	}

	template<std::input_iterator T>
	inline unsigned char Read(T &input_iterator)
	{
		const auto value = *input_iterator;
		++input_iterator;
		return value;
	};

	#if __STDC_HOSTED__ == 1
	inline unsigned char Read(std::istream &input)
	{
		return input.get();
	};
	#endif

	template<typename T>
	class Output
	{
	private:
		T &output;

	public:
		Output(T &output)
			: output(output)
		{}

		void Write(unsigned char value);
		void Fill(unsigned char value, unsigned int count);
		template<unsigned int maximum_count>
		void Copy(const unsigned int distance, const unsigned int count);
	};

	template<Internal::random_access_input_output_iterator T>
	class Output<T>
	{
	private:
		T &output_iterator;

	public:
		Output(T &output_iterator)
			: output_iterator(output_iterator)
		{}

		void Write(const unsigned char value)
		{
			*output_iterator = value;
			++output_iterator;
		};

		void Fill(const unsigned char value, const unsigned int count)
		{
			std::fill_n(output_iterator, count, value);
		}

		template<unsigned int maximum_count>
		void Copy(const unsigned int distance, const unsigned int count)
		{
			std::copy(output_iterator - distance, output_iterator - distance + count, output_iterator);
		};
	};

	#if __STDC_HOSTED__ == 1
	template<>
	class Output<std::iostream>
	{
	private:
		std::iostream &output;

	public:
		Output(std::iostream &output)
			: output(output)
		{}

		void Write(const unsigned char value)
		{
			output.put(value);
		};

		void Fill(const unsigned char value, const unsigned int count)
		{
			for (unsigned int i = 0; i < count; ++i)
				output.put(value);
		}

		template<unsigned int maximum_count>
		void Copy(const unsigned int offset, const unsigned int count)
		{
			std::array<char, maximum_count> bytes;

			const auto write_position = output.tellp();

			output.seekg(write_position);
			output.seekg(-static_cast<std::iostream::off_type>(offset), output.cur);
			output.read(bytes.data(), std::min(offset, count));

			for (unsigned int i = offset; i < count; ++i)
				bytes[i] = bytes[i - offset];

			output.seekp(write_position);
			output.write(bytes.data(), count);
		};
	};
	#endif

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
						bits |= Read(input);
					}
					else if constexpr(endian == Endian::Little)
					{
						bits >>= 8;
						bits |= static_cast<decltype(bits)>(Read(input)) << (total_bits - 8);
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

#endif // CLOWNLZSS_DECOMPRESSORS_COMMON_H
