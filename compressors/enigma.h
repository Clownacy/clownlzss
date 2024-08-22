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

#ifndef CLOWNLZSS_COMPRESSORS_ENIGMA_H
#define CLOWNLZSS_COMPRESSORS_ENIGMA_H

#include <algorithm>
#include <bit>
#include <utility>

#include "../bitfield.h"
#include "clownlzss.h"
#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace Enigma
		{
			template<typename T>
			using BitFieldWriter = BitField::Writer<1, BitField::WriteWhen::BeforePush, BitField::PushWhere::Low, BitField::Endian::Big, T>;

			template<typename T>
			inline bool Compress(const unsigned char* const data, const std::size_t data_size, CompressorOutput<T> &output)
			{
				constexpr unsigned int bytes_per_value = 2;

				if (data_size % bytes_per_value != 0)
					return false;

				constexpr auto ReadWord = [](const unsigned char* const input)
				{
					const unsigned int value = static_cast<unsigned int>(input[0]) << 8 | input[1];
					return value;
				};

				// Here, we determine the inline value length and render flag bitmask.
				// To begin with, we bitwise-OR all words together.
				unsigned int combined = 0;
				for (std::size_t i = 0; i < data_size; i += 2)
					combined |= ReadWord(data + i);

				const unsigned int inline_value_length = std::bit_width(combined & 0x7FF);
				const unsigned int render_flags_mask = combined >> (16 - 5);

				output.Write(inline_value_length);
				output.Write(render_flags_mask);
				output.WriteBE16(0);
				output.WriteBE16(0);

				BitFieldWriter bits(output);

				const unsigned char* const input_end = data + data_size;

				const auto GetWordsRemaining = [&](const unsigned char* const input)
				{
					return std::distance(input, input_end) / bytes_per_value;
				};

				for (const unsigned char *input = data; input < input_end; )
				{
					struct Run
					{
						enum class Type
						{
							Same = 4,
							Increment = 5,
							Decrement = 6
						};

						Type type;
						unsigned int length;
					};

					const auto GetRun = [&](const unsigned char* const input)
					{
						const unsigned int maximum_length = std::min<unsigned int>(0x10, GetWordsRemaining(input));

						const unsigned int first_value = ReadWord(input);

						const auto GetRunLength = [&]<typename Callback>(Callback callback)
						{
							unsigned int i;
							for (i = 1; i < maximum_length; ++i)
							{
								const unsigned int this_value = ReadWord(input + i * bytes_per_value);
								
								if (this_value != callback(i))
									break;
							}

							return i;
						};

						const auto LiteralRunCallback = [&]([[maybe_unused]] const unsigned int index)
						{
							return first_value;
						};

						const unsigned int literal_run_length = GetRunLength(LiteralRunCallback);

						const auto IncrementRunCallback = [&](const unsigned int index)
						{
							return first_value + index;
						};

						const unsigned int increment_run_length = GetRunLength(IncrementRunCallback);

						const auto DecrementRunCallback = [&](const unsigned int index)
						{
							return first_value - index;
						};

						const unsigned int decrement_run_length = GetRunLength(DecrementRunCallback);

						Run run;
						run.type = Run::Type::Same;
						run.length = literal_run_length;

						if (run.length < increment_run_length)
						{
							run.type = Run::Type::Increment;
							run.length = increment_run_length;
						}

						if (run.length < decrement_run_length)
						{
							run.type = Run::Type::Decrement;
							run.length = decrement_run_length;
						}

						return run;
					};

					const unsigned int maximum_length = std::min<unsigned int>(0xF, GetWordsRemaining(input));

					Run run;
					unsigned int raw_copy_length;
					for (raw_copy_length = 0; raw_copy_length < maximum_length; ++raw_copy_length)
					{
						run = GetRun(input + raw_copy_length * bytes_per_value);

						if (run.length != 1)
							break;
					}

					const auto WriteInlineValue = [&](const unsigned char* const input)
					{
						const unsigned int value = ReadWord(input);

						// Push render flag bits.
						for (unsigned int i = 0; i < 5; ++i)
							if ((render_flags_mask & 1 << (5 - i - 1)) != 0)
								bits.Push((value & 1 << (16 - i - 1)) != 0);

						// Push the tile index bits.
						bits.Push(value, inline_value_length);
					};

					if (raw_copy_length > 0)
					{
						bits.Push(7, 3);
						bits.Push(raw_copy_length - 1, 4);

						for (unsigned int i = 0; i < raw_copy_length; ++i)
							WriteInlineValue(input + i * bytes_per_value);
					}

					input += raw_copy_length * bytes_per_value;

					bits.Push(static_cast<unsigned int>(run.type), 3);
					bits.Push(run.length - 1, 4);
					WriteInlineValue(input);

					input += run.length * bytes_per_value;
				}

				// Write terminator pattern.
				bits.Push(7, 3);
				bits.Push(0xF, 4);

				return true;
			}
		}
	}

	template<typename T>
	bool EnigmaCompress(const unsigned char* const data, const std::size_t data_size, T &&output)
	{
		using namespace Internal;

		CompressorOutput output_wrapped(std::forward<T>(output));
		return Enigma::Compress(data, data_size, output_wrapped);
	}

	template<typename T>
	bool ModuledEnigmaCompress(const unsigned char* const data, const std::size_t data_size, T &&output, const std::size_t module_size)
	{
		using namespace Internal;

		CompressorOutput output_wrapped(std::forward<T>(output));
		return ModuledCompressionWrapper(data, data_size, output_wrapped, Enigma::Compress, module_size, 2);
	}
}

#endif // CLOWNLZSS_COMPRESSORS_ENIGMA_H
