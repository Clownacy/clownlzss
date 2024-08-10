#ifndef CLOWNLZSS_DECOMPRESSORS_COMMON_H
#define CLOWNLZSS_DECOMPRESSORS_COMMON_H

#include <algorithm>
#include <array>

namespace ClownLZSS
{
	template<std::input_or_output_iterator T>
	inline unsigned char Read(T &input_iterator)
	{
		const auto value = *input_iterator;
		++input_iterator;
		return value;
	};

	template<std::input_or_output_iterator T>
	inline void Write(T &output_iterator, const unsigned char value)
	{
		*output_iterator = value;
		++output_iterator;
	};

	template<std::input_or_output_iterator T>
	inline void Fill(T &output_iterator, const unsigned char value, const unsigned char count)
	{
		std::fill_n(output_iterator, count, value);
	}

	template<unsigned int maximum_count, std::input_or_output_iterator T>
	inline void Copy(T &output_iterator, const unsigned int distance, const unsigned int count)
	{
		std::copy(output_iterator - distance, output_iterator - distance + count, output_iterator);
	};

	#if __STDC_HOSTED__ == 1
	inline unsigned char Read(std::istream &input)
	{
		return input.get();
	};

	inline void Write(std::iostream &output, const unsigned char value)
	{
		output.put(value);
	};

	inline void Fill(std::iostream &output, const unsigned char value, const unsigned int count)
	{
		for (unsigned int i = 0; i < count; ++i)
			output.put(value);
	}

	template<unsigned int maximum_count>
	inline void Copy(std::iostream &output, const unsigned int offset, const unsigned int count)
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
	#endif
}

#endif // CLOWNLZSS_DECOMPRESSORS_COMMON_H
