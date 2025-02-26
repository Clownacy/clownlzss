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
#include <cstdlib>
#include <optional>
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
				if (data_size == 0)
					return true;

				constexpr unsigned int bytes_per_value = 2;

				if (data_size % bytes_per_value != 0)
					return false;

				constexpr auto ReadWord = [](const unsigned char* const input) constexpr
				{
					const unsigned int value = static_cast<unsigned int>(input[0]) << 8 | input[1];
					return value;
				};

				constexpr auto GetTileIndex = [](const unsigned int value) constexpr
				{
					return value & 0x7FF;
				};

				struct SpecialValues
				{
					unsigned int most_common;
					unsigned int lowest;
				};

				const auto FindSpecialValues = [&ReadWord, &GetTileIndex](const unsigned char* const data, const std::size_t data_size) -> std::optional<SpecialValues>
				{
					// Copy the input buffer.
					const std::size_t total_values = data_size / bytes_per_value;
					unsigned short* const sort_buffer = static_cast<unsigned short*>(std::malloc(total_values * sizeof(unsigned short)));

					if (sort_buffer == nullptr)
						return std::nullopt;

					for (std::size_t i = 0; i < data_size; i += bytes_per_value)
						sort_buffer[i / bytes_per_value] = ReadWord(data + i);

					// Emulate the bizarre quirk where the incremental copy word is set to zero if,
					// and only if, the data begins with a series of zeroes followed by a one.
					unsigned int lowest_value = 0xFFFF;

					for (std::size_t i = 1; i < total_values; ++i)
					{
						if (sort_buffer[i] == sort_buffer[i - 1] + 1)
						{
							if (GetTileIndex(sort_buffer[i - 1]) == 0)
								lowest_value = sort_buffer[0];
							break;
						}
					}

					// Sort the input buffer.
					std::sort(&sort_buffer[0], &sort_buffer[total_values]);

					// Find the longest run of a single value. This is the most common value.
					unsigned int run_length = 0;
					unsigned int run_value = sort_buffer[0];

					unsigned int longest_run_length = run_length;
					unsigned int longest_run_value = run_value;

					for (std::size_t i = 0; i < total_values; ++i)
					{
						const unsigned int this_value = sort_buffer[i];

						if (GetTileIndex(this_value) != 0) // Labyrinth Zone's 16x16 blocks rely on this odd masking.
							lowest_value = std::min(lowest_value, this_value);

						if (run_value == this_value)
						{
							++run_length;

							if (longest_run_length < run_length)
							{
								longest_run_length = run_length;
								longest_run_value = run_value;
							}
						}
						else
						{
							run_length = 1;
							run_value = this_value;
						}
					}

					std::free(sort_buffer);

					return SpecialValues{longest_run_value, lowest_value};
				};

				auto special_values = FindSpecialValues(data, data_size);

				if (!special_values.has_value())
					return false;

				// Here, we determine the inline value length and render flag bitmask.
				// To begin with, we bitwise-OR all words together.
				unsigned int combined = 0;
				for (std::size_t i = 0; i < data_size; i += bytes_per_value)
					combined |= ReadWord(data + i);

				const unsigned int inline_value_length = std::bit_width(GetTileIndex(combined));
				const unsigned int render_flags_mask = combined >> (16 - 5);

				output.Write(inline_value_length);
				output.Write(render_flags_mask);
				output.WriteBE16(special_values->lowest);
				output.WriteBE16(special_values->most_common);

				BitFieldWriter<decltype(output)> bits(output);

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
							Decrement = 6,
							Raw = 7
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
								
								if (callback(this_value, i))
									break;
							}

							return i;
						};

						const auto LiteralRunCallback = [&](const unsigned int this_value, [[maybe_unused]] const unsigned int index)
						{
							return this_value != first_value;
						};

						const unsigned int literal_run_length = GetRunLength(LiteralRunCallback);

						const auto IncrementRunCallback = [&](const unsigned int this_value, const unsigned int index)
						{
							return this_value != first_value + index || this_value == special_values->lowest;
						};

						const unsigned int increment_run_length = GetRunLength(IncrementRunCallback);

						const auto DecrementRunCallback = [&](const unsigned int this_value, const unsigned int index)
						{
							return this_value != first_value - index;
						};

						const unsigned int decrement_run_length = GetRunLength(DecrementRunCallback);

						// Always prefer special matches to inline matches, even if the inline matches are longer.
						if (first_value == special_values->lowest && increment_run_length)
						{
							Run run;
							run.type = Run::Type::Increment;
							run.length = increment_run_length;
							return run;
						}
						else if (first_value == special_values->most_common)
						{
							Run run;
							run.type = Run::Type::Same;
							run.length = literal_run_length;
							return run;
						}

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

					const auto IsIncrementalCopyMatch = [&](const Run &run, const unsigned char* const input)
					{
						return (run.type == Run::Type::Increment || run.length == 1) && ReadWord(input) == special_values->lowest;
					};

					const auto IsLiteralCopyMatch = [&](const Run &run, const unsigned char* const input)
					{
						return (run.type == Run::Type::Same || run.length == 1) && ReadWord(input) == special_values->most_common;
					};

					const unsigned int maximum_length = std::min<unsigned int>(0xF, GetWordsRemaining(input));

					Run run;
					unsigned int raw_copy_length;
					for (raw_copy_length = 0; raw_copy_length < maximum_length; ++raw_copy_length)
					{
						const auto run_input = input + raw_copy_length * bytes_per_value;

						run = GetRun(run_input);

						if (raw_copy_length == 0)
						{
							if (
								run.length > 1
								|| IsLiteralCopyMatch(run, run_input)
								|| IsIncrementalCopyMatch(run, run_input)
							)
								break;
						}
						else
						{
							if (run.length > 2 || (IsLiteralCopyMatch(run, run_input) && run.length > 1) || IsIncrementalCopyMatch(run, run_input))
								break;
						}
					}

					if (raw_copy_length != 0)
					{
						if (raw_copy_length == 1 && (maximum_length == 1 || IsIncrementalCopyMatch(run, input + bytes_per_value)))
							run.type = Run::Type::Same;
						else
							run.type = Run::Type::Raw;
						run.length = raw_copy_length;
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

					if (run.type == Run::Type::Raw)
					{
						bits.Push(static_cast<unsigned int>(run.type), 3);
						bits.Push(run.length - 1, 4);
						for (unsigned int i = 0; i < run.length; ++i)
							WriteInlineValue(input + i * bytes_per_value);
					}
					else if (IsIncrementalCopyMatch(run, input))
					{
						bits.Push(0, 2);
						bits.Push(run.length - 1, 4);
						special_values->lowest += run.length;
					}
					else if (IsLiteralCopyMatch(run, input))
					{
						bits.Push(1, 2);
						bits.Push(run.length - 1, 4);
					}
					else
					{
						bits.Push(static_cast<unsigned int>(run.type), 3);
						bits.Push(run.length - 1, 4);
						WriteInlineValue(input);
					}

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

		const auto start = output_wrapped.Tell();
		const bool success = Enigma::Compress(data, data_size, output_wrapped);

		if (output_wrapped.Distance(start) % 2 != 0)
			output_wrapped.Write(0);

		return success;
	}

	template<typename T>
	bool ModuledEnigmaCompress(const unsigned char* const data, const std::size_t data_size, T &&output, const std::size_t module_size)
	{
		using namespace Internal;

		CompressorOutput output_wrapped(std::forward<T>(output));
		return ModuledCompressionWrapper<2, Endian::Big>(data, data_size, output_wrapped, Enigma::Compress, module_size, 2);
	}
}

#endif // CLOWNLZSS_COMPRESSORS_ENIGMA_H
