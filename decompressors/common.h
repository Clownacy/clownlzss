/*
Copyright (c) 2018-2024 Clownacy

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef CLOWNLZSS_DECOMPRESSORS_COMMON_H
#define CLOWNLZSS_DECOMPRESSORS_COMMON_H

#include <algorithm>
#include <array>
#include <iterator>
#if __STDC_HOSTED__
	#include <istream>
	#include <ostream>
#endif
#include <type_traits>

#include "../common.h"

namespace ClownLZSS
{
	// DecompressorInputBase

	template<typename T, typename Derived>
	class DecompressorInputBase : public Internal::InputCommon<Derived>
	{
	public:
		DecompressorInputBase(T input);
	};

	template<typename T, typename Derived>
	requires std::input_iterator<std::decay_t<T>>
	class DecompressorInputBase<T, Derived> : public Internal::InputCommon<Derived>, public Internal::IOIteratorCommon<T>
	{
	protected:
		using Base = Internal::InputCommon<Derived>;

		using IOIteratorCommon = Internal::IOIteratorCommon<T>;
		using Iterator = IOIteratorCommon::Iterator;

		using IOIteratorCommon::iterator;

		unsigned char ReadImplementation()
		{
			const auto value = *iterator;
			++iterator;
			return value;
		}

	public:
		using Internal::InputCommon<Derived>::InputCommon;

		DecompressorInputBase(Iterator iterator)
			: IOIteratorCommon::IOIteratorCommon(iterator)
		{}

		DecompressorInputBase& operator+=(const unsigned int value)
		{
			iterator += value;
			return *this;
		}

		friend Base;
	};

	#if __STDC_HOSTED__
	template<typename T, typename Derived>
	requires std::is_convertible_v<T&, std::istream&>
	class DecompressorInputBase<T, Derived> : public Internal::InputCommon<Derived>
	{
	protected:
		using Base = Internal::InputCommon<Derived>;

		std::istream &input;

		unsigned char ReadImplementation()
		{
			return input.get();
		}

		DecompressorInputBase& AdditionAssignImplementation(const unsigned int value)
		{
			input.seekg(value, input.cur);
			return *this;
		}

	public:
		using pos_type = std::istream::pos_type;
		using difference_type = std::istream::off_type;

		using Internal::InputCommon<Derived>::InputCommon;

		DecompressorInputBase(std::istream &input)
			: input(input)
		{}

		DecompressorInputBase& operator+=(const unsigned int value)
		{
			return static_cast<Derived*>(this)->AdditionAssignImplementation(value);
		}

		pos_type Tell() const
		{
			return input.tellg();
		};

		void Seek(const pos_type &position)
		{
			input.seekg(position);
		};

		difference_type Distance(const pos_type &first) const
		{
			return Distance(first, Tell());
		}

		static difference_type Distance(const pos_type &first, const pos_type &last)
		{
			return last - first;
		}

		friend Base;
	};
	#endif

	// DecompressorInput

	template<typename T>
	class DecompressorInput : public DecompressorInputBase<T, DecompressorInput<T>>
	{
	public:
		DecompressorInput(T input);
	};

	template<typename T>
	requires std::random_access_iterator<std::decay_t<T>>
	class DecompressorInput<T> : public DecompressorInputBase<T, DecompressorInput<T>>
	{
	public:
		using DecompressorInputBase<T, DecompressorInput<T>>::DecompressorInputBase;
	};

	#if __STDC_HOSTED__
	template<typename T>
	requires std::is_convertible_v<T&, std::istream&>
	class DecompressorInput<T> : public DecompressorInputBase<T, DecompressorInput<T>>
	{
	public:
		using DecompressorInputBase<T, DecompressorInput<T>>::DecompressorInputBase;
	};
	#endif

	// DecompressorInputSeparate

	template<typename T>
	class DecompressorInputSeparate : public DecompressorInputBase<T, DecompressorInputSeparate<T>>
	{
	public:
		DecompressorInputSeparate(T input);
	};

	template<typename T>
	requires std::random_access_iterator<std::decay_t<T>>
	class DecompressorInputSeparate<T> : public DecompressorInputBase<T, DecompressorInputSeparate<T>>
	{
	public:
		using DecompressorInputBase<T, DecompressorInputSeparate<T>>::DecompressorInputBase;
	};

	#if __STDC_HOSTED__
	template<typename T>
	requires std::is_convertible_v<T&, std::istream&>
	class DecompressorInputSeparate<T> : public DecompressorInputBase<T, DecompressorInputSeparate<T>>
	{
	protected:
		using Base = DecompressorInputBase<T, DecompressorInputSeparate<T>>;

		Base::pos_type position;

		unsigned char ReadImplementation()
		{
			const auto previous_position = Base::Tell();
			Base::Seek(position);
			const auto value = Base::ReadImplementation();
			position = Base::Tell();
			Base::Seek(previous_position);
			return value;
		}

		DecompressorInputSeparate& AdditionAssignImplementation(const unsigned int value)
		{
			const auto previous_position = Base::Tell();
			Base::Seek(position);
			Base::AdditionAssignImplementation(value);
			position = Base::Tell();
			Base::Seek(previous_position);
			return *this;
		}

	public:
		DecompressorInputSeparate(std::istream &input)
			: Base::DecompressorInputBase(input)
			, position(Base::Tell())
		{}

		friend Base;
		friend Internal::InputCommon<DecompressorInputSeparate<T>>;
	};
	#endif

	// DecompressorOutput

	template<typename T, unsigned int dictionary_size, unsigned int maximum_copy_length, int filler_value = -1>
	class DecompressorOutput : public Internal::OutputCommon<T, DecompressorOutput<T, dictionary_size, maximum_copy_length>>
	{
	public:
		DecompressorOutput(T output);
	};

	template<typename T, unsigned int dictionary_size, unsigned int maximum_copy_length, int filler_value>
	requires Internal::random_access_input_output_iterator<std::decay_t<T>>
	class DecompressorOutput<T, dictionary_size, maximum_copy_length, filler_value> : public Internal::OutputCommon<T, DecompressorOutput<T, dictionary_size, maximum_copy_length>>
	{
	protected:
		using Base = Internal::OutputCommon<T, DecompressorOutput<T, dictionary_size, maximum_copy_length>>;
		using Iterator = Base::Iterator;
		using Base::iterator;

		Iterator start_iterator;

	public:
		using Base::OutputCommon;

		DecompressorOutput(Iterator iterator)
			: Base::OutputCommon(iterator)
		{
			if constexpr(filler_value != -1)
				start_iterator = iterator;
		}

		void Copy(const unsigned int distance, const unsigned int count)
		{
			if constexpr(filler_value != -1)
			{
				const unsigned int limit = Base::Distance(start_iterator);
				const unsigned int capped_distance = std::min(distance, limit);
				const unsigned int fill_amount = distance - capped_distance;

				Base::Fill(filler_value, fill_amount);
				std::copy(iterator - distance + fill_amount, iterator - distance + count, iterator);
			}
			else
			{
				std::copy(iterator - distance, iterator - distance + count, iterator);
			}

			iterator += count;
		}
	};

	#if __STDC_HOSTED__
	template<typename T, unsigned int dictionary_size, unsigned int maximum_copy_length, int filler_value>
	requires std::is_convertible_v<T&, std::ostream&>
	class DecompressorOutput<T, dictionary_size, maximum_copy_length, filler_value> : public Internal::OutputCommon<T, DecompressorOutput<T, dictionary_size, maximum_copy_length>>
	{
	protected:
		using Base = Internal::OutputCommon<T, DecompressorOutput<T, dictionary_size, maximum_copy_length>>;
		using Base::output;

		std::array<char, dictionary_size + maximum_copy_length - 1> buffer;
		unsigned int index = 0;

		void WriteToBuffer(const unsigned char value)
		{
			buffer[index] = value;

			// A lovely little trick that is borrowed from Okumura's LZSS decompressor...
			if (index < maximum_copy_length)
				buffer[dictionary_size + index] = value;

			index = (index + 1) % dictionary_size;
		}

		void WriteImplementation(const unsigned char value)
		{
			WriteToBuffer(value);
			Base::WriteImplementation(value);
		}

	public:
		DecompressorOutput(std::ostream &output)
			: Base::OutputCommon(output)
		{
			if constexpr(filler_value != -1)
				buffer.fill(filler_value);
		}

		void Copy(const unsigned int distance, const unsigned int count)
		{
			const unsigned int source_index = (index - distance + dictionary_size) % dictionary_size;
			const unsigned int destination_index = index;

			for (unsigned int i = 0; i < count; ++i)
				WriteToBuffer(buffer[source_index + i]);

			output.write(&buffer[destination_index], count);
		}

		friend Base;
	};
	#endif
}

#endif // CLOWNLZSS_DECOMPRESSORS_COMMON_H
