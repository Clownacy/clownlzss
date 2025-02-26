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

#ifndef CLOWNLZSS_BITFIELD_H
#define CLOWNLZSS_BITFIELD_H

#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace BitField
		{
			enum class ReadWhen
			{
				BeforePop,
				AfterPop
			};

			enum class WriteWhen
			{
				BeforePush,
				AfterPush
			};

			enum class PopWhere
			{
				Low,
				High
			};

			enum class PushWhere
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
			requires (total_bytes >= 1) && (total_bytes <= 4)
			class Reader
			{
			private:
				static constexpr unsigned int total_bits = total_bytes * 8;

				Input &input;
				unsigned long bits = 0;
				unsigned int bits_remaining;

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
				Reader(Input &input)
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

				unsigned int Pop(const unsigned int total_bits)
				{
					unsigned int value = 0;

					for (unsigned int i = 0; i < total_bits; ++i)
					{
						value <<= 1;
						value |= Pop();
					}

					return value;
				}
			};

			template<unsigned int total_bytes, WriteWhen write_when, PushWhere push_where, Endian endian, typename Output, typename Derived>
			requires (total_bytes >= 1) && (total_bytes <= 4)
			class WriterBase
			{
			protected:
				static constexpr unsigned int total_bits = total_bytes * 8;

				Output &output;
				unsigned long bits = 0;
				unsigned int bits_remaining = total_bits;

				void WriteBitsImplementation()
				{
					for (unsigned int i = 0; i < total_bytes; ++i)
					{
						unsigned int shift;

						if constexpr(endian == Endian::Big)
							shift = total_bytes - i - 1;
						else //if constexpr(endian == Endian::Little)
							shift = i;

						output.Write((bits >> (shift * 8)) & 0xFF);
					}

					bits_remaining = total_bits;
				}

				void WriteBits()
				{
					static_cast<Derived*>(this)->WriteBitsImplementation();
				}

				void Flush()
				{
					if (bits_remaining != total_bits)
					{
						if constexpr(push_where == PushWhere::High)
							bits >>= bits_remaining;
						else //if constexpr(push_where == PushWhere::Low)
							bits <<= bits_remaining;

						WriteBitsImplementation();
					}
				}

			public:
				WriterBase(Output &output)
					: output(output)
				{}

				~WriterBase()
				{
					Flush();
				}

				void Push(const bool bit)
				{
					const auto &CheckWriteBits = [&]()
					{
						if (bits_remaining == 0)
						{
							WriteBits();
						}
					};

					if constexpr(write_when == WriteWhen::BeforePush)
						CheckWriteBits();

					if constexpr(push_where == PushWhere::High)
					{
						bits >>= 1;
						bits |= bit << (total_bits - 1);
					}
					else //if constexpr(push_where == PushWhere::Low)
					{
						bits <<= 1;
						bits |= bit;
					}

					--bits_remaining;

					if constexpr(write_when == WriteWhen::AfterPush)
						CheckWriteBits();
				}

				void Push(const unsigned int value, const unsigned int total_bits)
				{
					for (unsigned int i = 0; i < total_bits; ++i)
						Push((value & 1 << (total_bits - i - 1)) != 0);
				}
			};

			template<unsigned int total_bytes, WriteWhen write_when, PushWhere push_where, Endian endian, typename Output>
			class Writer : public WriterBase<total_bytes, write_when, push_where, endian, Output, Writer<total_bytes, write_when, push_where, endian, Output>>
			{
			public:
				Writer(Output &output)
					: WriterBase<total_bytes, write_when, push_where, endian, Output, Writer<total_bytes, write_when, push_where, endian, Output>>::WriterBase(output)
				{}
			};

			template<unsigned int total_bytes, WriteWhen write_when, PushWhere push_where, Endian endian, typename Output>
			class DescriptorFieldWriter : public WriterBase<total_bytes, write_when, push_where, endian, Output, DescriptorFieldWriter<total_bytes, write_when, push_where, endian, Output>>
			{
			protected:
				using Base = WriterBase<total_bytes, write_when, push_where, endian, Output, DescriptorFieldWriter<total_bytes, write_when, push_where, endian, Output>>;

				using Base::output;

				typename std::remove_cvref_t<Output>::pos_type descriptor_position;

				void Begin()
				{
					// Log the placement of the descriptor field.
					descriptor_position = output.Tell();

					// Insert a placeholder.
					output.Fill(0, total_bytes);
				}

				template<typename T>
				void Finish(T &&function)
				{
					// Back up current position.
					const auto current_position = output.Tell();

					// Go back to the descriptor field.
					output.Seek(descriptor_position);

					// Write the complete descriptor field.
					function();

					// Seek back to where we were before.
					output.Seek(current_position);
				}

				void WriteBitsImplementation()
				{
					Finish([this](){Base::WriteBitsImplementation();});
					Begin();
				}

			public:
				DescriptorFieldWriter(Output &output)
					: Base::WriterBase(output)
				{
					Begin();
				}

				~DescriptorFieldWriter()
				{
					Finish([this](){Base::Flush();});
				}

				friend Base;
			};
		}
	}
}

#endif // CLOWNLZSS_BITFIELD_H
