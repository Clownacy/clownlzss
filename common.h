/*
Copyright (c) 2018-2022 Clownacy

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

#ifndef CLOWNLZSS_COMMON_H
#define CLOWNLZSS_COMMON_H

#include <iterator>
#if __STDC_HOSTED__ == 1
	#include <ostream>
#endif
#include <type_traits>

namespace ClownLZSS
{
	namespace Internal
	{
		template<typename T>
		concept random_access_input_output_iterator = std::random_access_iterator<T> && std::output_iterator<T, unsigned char>;

		template<typename T>
		class IOIteratorCommon
		{
		protected:
			using Iterator = std::decay_t<T>;

			Iterator iterator;

		public:
			using pos_type = Iterator;
			using difference_type = std::iterator_traits<Iterator>::difference_type;

			IOIteratorCommon(Iterator iterator)
				: iterator(iterator)
			{}

			pos_type Tell() const
			{
				return iterator;
			};

			void Seek(const pos_type &position)
			{
				iterator = position;
			};

			difference_type Distance(const pos_type &first) const
			{
				return Distance(first, Tell());
			}

			static difference_type Distance(const pos_type &first, const pos_type &last)
			{
				return std::distance(first, last);
			}
		};

		template<typename Derived>
		class InputCommon
		{
		public:
			unsigned char Read()
			{
				return static_cast<Derived*>(this)->ReadImplementation();
			}

			unsigned int ReadBE16()
			{
				const unsigned int upper = Read();
				const unsigned int lower = Read();
				return upper << 8 | lower;
			}

			unsigned int ReadLE16()
			{
				const unsigned int lower = Read();
				const unsigned int upper = Read();
				return upper << 8 | lower;
			}
		};

		template<typename T, typename Derived>
		class OutputCommon
		{
		public:
			OutputCommon(T output) = delete;
		};

		template<typename T, typename Derived>
		requires Internal::random_access_input_output_iterator<std::decay_t<T>>
		class OutputCommon<T, Derived> : public IOIteratorCommon<T>
		{
		protected:
			using Iterator = IOIteratorCommon<T>::Iterator;

			using IOIteratorCommon<T>::iterator;

			void WriteImplementation(const unsigned char value)
			{
				*iterator = value;
				++iterator;
			}

		public:
			OutputCommon(Iterator iterator)
				: IOIteratorCommon<T>(iterator)
			{}

			void Write(const unsigned char value)
			{
				static_cast<Derived*>(this)->WriteImplementation(value);
			}

			void Fill(const unsigned char value, const unsigned int count)
			{
				std::fill_n(iterator, count, value);
				iterator += count;
			}
		};

		#if __STDC_HOSTED__ == 1
		template<typename T, typename Derived>
		requires std::is_convertible_v<T&, std::ostream&>
		class OutputCommon<T, Derived>
		{
		protected:
			std::ostream &output;

			void WriteImplementation(const unsigned char value)
			{
				output.put(value);
			}

		public:
			using pos_type = std::ostream::pos_type;
			using difference_type = std::ostream::off_type;

			OutputCommon(std::ostream &output)
				: output(output)
			{}

			void Write(const unsigned char value)
			{
				static_cast<Derived*>(this)->WriteImplementation(value);
			}

			void Fill(const unsigned char value, const unsigned int count)
			{
				for (unsigned int i = 0; i < count; ++i)
					Write(value);
			}

			pos_type Tell() const
			{
				return output.tellp();
			};

			void Seek(const pos_type &position)
			{
				output.seekp(position);
			};

			difference_type Distance(const pos_type &first) const
			{
				return Distance(first, Tell());
			}

			static difference_type Distance(const pos_type &first, const pos_type &last)
			{
				return last - first;
			}
		};
		#endif
	}
}

#endif // CLOWNLZSS_COMMON_H
