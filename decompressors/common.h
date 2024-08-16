#ifndef CLOWNLZSS_DECOMPRESSORS_COMMON_H
#define CLOWNLZSS_DECOMPRESSORS_COMMON_H

#include <algorithm>
#include <array>
#include <iterator>

#include "bitfield.h"

namespace ClownLZSS
{
	namespace Internal
	{
		template<typename T>
		concept random_access_input_output_iterator = std::random_access_iterator<T> && std::output_iterator<T, unsigned char>;

		template<typename T>
		class OutputCommon
		{
		public:
			OutputCommon(T output) = delete;
		};

		template<typename T>
		requires Internal::random_access_input_output_iterator<std::decay_t<T>>
		class OutputCommon<T>
		{
		protected:
			std::decay_t<T> output_iterator;

		public:
			OutputCommon(std::decay_t<T> output_iterator)
				: output_iterator(output_iterator)
			{}

			void Write(const unsigned char value)
			{
				*output_iterator = value;
				++output_iterator;
			};
		};

		#if __STDC_HOSTED__ == 1
		template<typename T>
		requires std::is_convertible_v<T&, std::ostream&>
		class OutputCommon<T>
		{
		protected:
			std::ostream &output;

		public:
			OutputCommon(std::ostream &output)
				: output(output)
			{}

			void Write(const unsigned char value)
			{
				output.put(value);
			};
		};
		#endif
	}

	// DecompressorInput

	template<typename T>
	class DecompressorInput
	{
	public:
		DecompressorInput(T input) = delete;
	};

	template<typename T>
	class DecompressorInputWithLength : public DecompressorInput<T>
	{
	public:
		DecompressorInputWithLength(T input, unsigned int length) = delete;
	};

	template<typename T>
	requires std::input_iterator<std::decay_t<T>>
	class DecompressorInput<T>
	{
	protected:
		std::decay_t<T> input_iterator;

	public:
		DecompressorInput(std::decay_t<T> input_iterator)
			: input_iterator(input_iterator)
		{}

		unsigned char Read()
		{
			const auto value = *input_iterator;
			++input_iterator;
			return value;
		}

		DecompressorInput& operator+=(const unsigned int value)
		{
			input_iterator += value;
			return *this;
		}
	};

	template<typename T>
	requires std::random_access_iterator<std::decay_t<T>>
	class DecompressorInputWithLength<T> : public DecompressorInput<T>
	{
	protected:
		std::decay_t<T> input_end;

	public:
		DecompressorInputWithLength(std::decay_t<T> input_iterator, unsigned int length)
			: DecompressorInput<T>(input_iterator)
			, input_end(input_iterator + length)
		{}

		bool AtEnd() const
		{
			return DecompressorInput<T>::input_iterator >= input_end;
		}
	};

	#if __STDC_HOSTED__ == 1
	template<typename T>
	requires std::is_convertible_v<T&, std::istream&>
	class DecompressorInput<T>
	{
	protected:
		std::istream &input;

	public:
		DecompressorInput(std::istream &input)
			: input(input)
		{}

		unsigned char Read()
		{
			return input.get();
		}

		DecompressorInput& operator+=(const unsigned int value)
		{
			input.seekg(value, input.cur);
			return *this;
		}
	};

	template<typename T>
	requires std::is_convertible_v<T&, std::istream&>
	class DecompressorInputWithLength<T> : public DecompressorInput<T>
	{
	protected:
		unsigned int position = 0, length;

	public:
		DecompressorInputWithLength(std::istream &input, unsigned int length)
			: DecompressorInput<T>(input)
			, length(length)
		{}

		unsigned char Read()
		{
			++position;
			return DecompressorInput<T>::Read();
		}

		bool AtEnd() const
		{
			return position >= length;
		}
	};
	#endif

	template<typename T>
	class DecompressorInputSeparate : public DecompressorInput<T>
	{
	public:
		DecompressorInputSeparate(T input) = delete;
	};

	template<typename T>
	requires std::random_access_iterator<std::decay_t<T>>
	class DecompressorInputSeparate<T> : public DecompressorInput<T>
	{
	public:
		using DecompressorInput<T>::DecompressorInput;
	};

	#if __STDC_HOSTED__ == 1
	template<typename T>
	requires std::is_convertible_v<T&, std::istream&>
	class DecompressorInputSeparate<T> : public DecompressorInput<T>
	{
	protected:
		std::istream::pos_type position;

	public:
		DecompressorInputSeparate(std::istream &input)
			: DecompressorInput<T>::DecompressorInput(input)
			, position(input.tellg())
		{}

		unsigned char Read()
		{
			const auto previous_position = DecompressorInput<T>::input.tellg();
			DecompressorInput<T>::input.seekg(position);
			const auto value = DecompressorInput<T>::Read();
			position = DecompressorInput<T>::input.tellg();
			DecompressorInput<T>::input.seekg(previous_position);
			return value;
		}

		DecompressorInputSeparate& operator+=(const unsigned int value)
		{
			const auto previous_position = DecompressorInput<T>::input.tellg();
			DecompressorInput<T>::input.seekg(position);
			DecompressorInput<T>::operator+=(value);
			position = DecompressorInput<T>::input.tellg();
			DecompressorInput<T>::input.seekg(previous_position);
			return *this;
		}
	};
	#endif

	// DecompressorOutput

	template<typename T, unsigned int dictionary_size, unsigned int maximum_copy_length>
	class DecompressorOutput
	{
	public:
		DecompressorOutput(T output) = delete;
	};

	template<typename T, unsigned int dictionary_size, unsigned int maximum_copy_length>
	requires Internal::random_access_input_output_iterator<std::decay_t<T>>
	class DecompressorOutput<T, dictionary_size, maximum_copy_length> : public Internal::OutputCommon<T>
	{
	public:
		using pos_type = std::decay_t<T>;

		using Internal::OutputCommon<T>::output_iterator;
		using Internal::OutputCommon<T>::OutputCommon;

		pos_type Tell()
		{
			return output_iterator;
		};

		void Seek(const pos_type &position)
		{
			output_iterator = position;
		};

		void Fill(const unsigned char value, const unsigned int count)
		{
			std::fill_n(output_iterator, count, value);
			output_iterator += count;
		}

		void Copy(const unsigned int distance, const unsigned int count)
		{
			std::copy(output_iterator - distance, output_iterator - distance + count, output_iterator);
			output_iterator += count;
		};
	};

	#if __STDC_HOSTED__ == 1
	template<typename T, unsigned int dictionary_size, unsigned int maximum_copy_length>
	requires std::is_convertible_v<T&, std::ostream&>
	class DecompressorOutput<T, dictionary_size, maximum_copy_length> : public Internal::OutputCommon<T>
	{
	protected:
		std::array<char, dictionary_size + maximum_copy_length - 1> buffer;
		unsigned int index = 0;

		void WriteToBuffer(const unsigned char value)
		{
			buffer[index] = value;

			// A lovely little trick that is borrowed from Okumura's LZSS decompressor...
			if (index < maximum_copy_length)
				buffer[dictionary_size + index] = value;

			index = (index + 1) % dictionary_size;
		};

	public:
		using Internal::OutputCommon<T>::output;
		using Internal::OutputCommon<T>::OutputCommon;

		void Write(const unsigned char value)
		{
			WriteToBuffer(value);
			Internal::OutputCommon<T>::Write(value);
		};

		void Fill(const unsigned char value, const unsigned int count)
		{
			for (unsigned int i = 0; i < count; ++i)
				Write(value);
		}

		void Copy(const unsigned int distance, const unsigned int count)
		{
			const unsigned int source_index = (index - distance + dictionary_size) % dictionary_size;
			const unsigned int destination_index = index;

			for (unsigned int i = 0; i < count; ++i)
				WriteToBuffer(buffer[source_index + i]);

			output.write(&buffer[destination_index], count);
		};
	};
	#endif

	// CompressorOutput

	template<typename T>
	class CompressorOutput
	{
	public:
		CompressorOutput(T output) = delete;
	};

	template<typename T>
	requires Internal::random_access_input_output_iterator<std::decay_t<T>>
	class CompressorOutput<T> : public Internal::OutputCommon<T>
	{
	public:
		using pos_type = std::decay_t<T>;

		using Internal::OutputCommon<T>::output_iterator;
		using Internal::OutputCommon<T>::OutputCommon;

		pos_type Tell()
		{
			return output_iterator;
		};

		void Seek(const pos_type &position)
		{
			output_iterator = position;
		};
	};

	#if __STDC_HOSTED__ == 1
	template<typename T>
	requires std::is_convertible_v<T&, std::ostream&>
	class CompressorOutput<T> : public Internal::OutputCommon<T>
	{
	public:
		using pos_type = std::ostream::pos_type;

		using Internal::OutputCommon<T>::output;
		using Internal::OutputCommon<T>::OutputCommon;

		pos_type Tell()
		{
			return output.tellp();
		};

		void Seek(const pos_type &position)
		{
			output.seekp(position);
		};
	};
	#endif
}

#endif // CLOWNLZSS_DECOMPRESSORS_COMMON_H
