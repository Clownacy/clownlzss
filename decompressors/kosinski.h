#ifndef CLOWNLZSS_DECOMPRESSORS_KOSINSKI_H
#define CLOWNLZSS_DECOMPRESSORS_KOSINSKI_H

#include <algorithm>
#include <array>

template<std::invocable T1, std::invocable<unsigned char> T2, std::invocable<unsigned int, unsigned int> T3>
void ClownLZSS_KosinskiDecompress(const T1 &read_callback, const T2 &write_callback, const T3 &copy_callback)
{
	unsigned int descriptor_bits_remaining;
	unsigned short descriptor_bits;

	const auto GetDescriptor = [&]()
	{
		const unsigned int low_byte = read_callback();
		const unsigned int high_byte = read_callback();

		descriptor_bits = (high_byte << 8) | low_byte;
		descriptor_bits_remaining = 16;

	};

	const auto PopDescriptor = [&]()
	{
		const bool result = (descriptor_bits & 1) != 0;

		descriptor_bits >>= 1;

		if (--descriptor_bits_remaining == 0)
			GetDescriptor();

		return result;
	};

	GetDescriptor();

	for (;;)
	{
		if (PopDescriptor())
		{
			write_callback(read_callback());
		}
		else
		{
			unsigned int offset;
			unsigned int count;

			if (PopDescriptor())
			{
				const unsigned int low_byte = read_callback();
				const unsigned int high_byte = read_callback();

				offset = ((high_byte & 0xF8) << 5) | low_byte;
				offset = 0x2000 - offset; // Convert from negative two's-complement to positive.
				count = high_byte & 7;

				if (count != 0)
				{
					count += 2;
				}
				else
				{
					count = read_callback() + 1;

					if (count == 1)
						break;
					else if (count == 2)
						continue;
				}
			}
			else
			{
				count = 2;

				if (PopDescriptor())
					count += 2;
				if (PopDescriptor())
					count += 1;

				offset = 0x100 - read_callback(); // Convert from negative two's-complement to positive.
			}

			copy_callback(offset, count);
		}
	}
}

template<std::input_or_output_iterator T1, std::input_or_output_iterator T2>
void ClownLZSS_KosinskiDecompress(T1 input_iterator, T2 output_iterator)
{
	const auto read_callback = [&]() -> unsigned char
	{
		const auto value = *input_iterator;
		++input_iterator;
		return value;
	};

	const auto write_callback = [&](const unsigned char value)
	{
		*output_iterator = value;
		++output_iterator;
	};

	const auto copy_callback = [&](const unsigned int distance, const unsigned int count)
	{
		std::copy(output_iterator - distance, output_iterator - distance + count, output_iterator);
	};

	ClownLZSS_KosinskiDecompress(read_callback, write_callback, copy_callback);
}

void ClownLZSS_KosinskiDecompress(std::istream &input, std::iostream &output)
{
	const auto read_callback = [&]() -> unsigned char
	{
		return input.get();
	};

	const auto write_callback = [&](const unsigned char value)
	{
		output.put(value);
	};

	const auto copy_callback = [&]<unsigned int maximum_count>(const unsigned int offset, const unsigned int count)
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

	const auto copy_callback_wrapper = [&](const unsigned int offset, const unsigned int count)
	{
		copy_callback.template operator()<0x100>(offset, count);
	};

	ClownLZSS_KosinskiDecompress(read_callback, write_callback, copy_callback_wrapper);
}

#endif /* CLOWNLZSS_DECOMPRESSORS_KOSINSKI_H */
