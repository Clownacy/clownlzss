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

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "compressors/chameleon.h"
#include "compressors/comper.h"
#include "compressors/faxman.h"
#include "compressors/kosinski.h"
#include "compressors/kosinskiplus.h"
#include "compressors/rage.h"
#include "compressors/rocket.h"
#include "compressors/saxman.h"

#include "decompressors/chameleon.h"
#include "decompressors/comper.h"
#include "decompressors/kosinski.h"
#include "decompressors/kosinskiplus.h"
#include "decompressors/saxman.h"

enum class Format
{
	CHAMELEON,
	COMPER,
	FAXMAN,
	KOSINSKI,
	KOSINSKIPLUS,
	RAGE,
	ROCKET,
	SAXMAN,
	SAXMAN_NO_HEADER
};

struct Mode
{
	std::string command;
	Format format;
	std::string normal_default_filename;
	std::string moduled_default_filename;
};

static const auto modes = std::to_array<Mode>({
	{"-ch", Format::CHAMELEON,        "out.cham", "out.chamm"},
	{"-c",  Format::COMPER,           "out.comp", "out.compm"},
	{"-f",  Format::FAXMAN,           "out.fax",  "out.faxm" },
	{"-k",  Format::KOSINSKI,         "out.kos",  "out.kosm" },
	{"-kp", Format::KOSINSKIPLUS,     "out.kosp", "out.kospm"},
	{"-ra", Format::RAGE,             "out.rage", "out.ragem"},
	{"-r",  Format::ROCKET,           "out.rock", "out.rockm"},
	{"-s",  Format::SAXMAN,           "out.sax",  "out.saxm" },
	{"-sn", Format::SAXMAN_NO_HEADER, "out.sax",  "out.saxm" }
});

static void PrintUsage(void)
{
	std::cout <<
		"Clownacy's LZSS compression tool\n"
		"\n"
		"Usage: clownlzss [options] [in-filename] [out-filename]"
		"\n"
		"Options:\n"
		"\n"
		" Format:\n"
		"  -ch    Chameleon\n"
		"  -c     Comper\n"
		"  -f     Faxman\n"
		"  -k     Kosinski\n"
		"  -kp    Kosinski+\n"
		"  -ra    Rage\n"
		"  -r     Rocket\n"
		"  -s     Saxman\n"
		"  -sn    Saxman (with no header)\n"
		"\n"
		" Misc:\n"
		"  -m[=MODULE_SIZE]  Compresses into modules\n"
		"                    MODULE_SIZE controls the module size (defaults to 0x1000)\n"
		"  -d     Decompress\n"
	;
}

static auto FileToBuffer(const std::filesystem::path &path)
{
	std::vector<unsigned char> buffer(std::filesystem::file_size(path));

	std::ifstream file;
	file.exceptions(file.badbit | file.eofbit | file.failbit);
	file.open(path, file.in | file.binary);

	file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

	return buffer;
}

int main(int argc, char **argv)
{
	int exit_code = EXIT_SUCCESS;

	const Mode *mode = NULL;
	std::filesystem::path in_filename;
	std::filesystem::path out_filename;
	bool moduled = false, decompress = false;
	std::size_t module_size = 0x1000;

	/* Skip past the executable name */
	--argc;
	++argv;

	/* Parse arguments */
	for (int i = 0; i < argc; ++i)
	{
		const std::string_view arg(argv[i]);

		if (arg[0] == '-')
		{
			if (arg == "-h" || arg == "--help")
			{
				PrintUsage();
			}
			else if (arg[1] == 'm')
			{
				moduled = true;

				const auto argument_position = arg.find_first_of('=');

				if (argument_position != arg.npos)
				{
					char *end;
					unsigned long result = std::strtoul(&argv[i][argument_position + 1], &end, 0);

					if (*end != '\0')
					{
						std::cerr << "Invalid parameter to -m\n";
						exit_code = EXIT_FAILURE;
						break;
					}
					else
					{
						module_size = result;

						if (module_size > 0x1000)
							std::cerr << "Warning: the moduled format header does not fully support sizes greater than\n 0x1000 - header will likely be invalid!\n";
					}
				}
			}
			else if (arg == "-d")
			{
				decompress = true;
			}
			else
			{
				for (const auto &current_mode : modes)
				{
					if (arg == current_mode.command)
					{
						mode = &current_mode;
						break;
					}
				}
			}
		}
		else
		{
			if (in_filename.empty())
				in_filename = arg;
			else
				out_filename = arg;
		}
	}

	if (exit_code != EXIT_FAILURE)
	{
		if (in_filename.empty())
		{
			exit_code = EXIT_FAILURE;
			std::cerr << "Error: Input file not specified\n";
			PrintUsage();
		}
		else if (mode == NULL)
		{
			exit_code = EXIT_FAILURE;
			std::cerr << "Error: Format not specified\n";
			PrintUsage();
		}
		else
		{
			if (out_filename.empty())
				out_filename = moduled ? mode->moduled_default_filename : mode->normal_default_filename;

			std::ofstream out_file;
			out_file.exceptions(out_file.badbit | out_file.eofbit | out_file.failbit);
			out_file.open(out_filename, decompress ? out_file.trunc | out_file.in | out_file.out | out_file.binary : out_file.out | out_file.binary);

			if (decompress)
			{
				std::ifstream in_file;
				in_file.exceptions(in_file.badbit | in_file.eofbit | in_file.failbit);
				in_file.open(in_filename, in_file.in | in_file.binary);

				switch (mode->format)
				{
					case Format::CHAMELEON:
						ClownLZSS::ChameleonDecompress(in_file, out_file);
						break;

					case Format::COMPER:
						ClownLZSS::ComperDecompress(in_file, out_file);
						break;

					case Format::KOSINSKI:
						ClownLZSS::KosinskiDecompress(in_file, out_file);
						break;

					case Format::KOSINSKIPLUS:
						ClownLZSS::KosinskiPlusDecompress(in_file, out_file);
						break;

					case Format::SAXMAN:
					{
						const unsigned int uncompressed_length_lower_byte = in_file.get();
						const unsigned int uncompressed_length_upper_byte = in_file.get();
						const unsigned int uncompressed_length = uncompressed_length_upper_byte << 8 | uncompressed_length_lower_byte;
						ClownLZSS::SaxmanDecompress(in_file, out_file, uncompressed_length);
						break;
					}

					case Format::SAXMAN_NO_HEADER:
					{
						ClownLZSS::SaxmanDecompress(in_file, out_file, std::filesystem::file_size(in_filename));
						break;
					}
				}
			}
			else
			{
				const bool success = [&]() -> bool
				{
					constexpr auto WriteCallback = [](void* const user_data, const unsigned char byte)
					{
						std::ofstream &file = *static_cast<std::ofstream*>(user_data);

						file.put(byte);
					};

					constexpr auto SeekCallback = [](void* const user_data, const std::size_t position)
					{
						std::ofstream &file = *static_cast<std::ofstream*>(user_data);

						file.seekp(position, file.beg);
					};

					constexpr auto TellCallback = [](void* const user_data)
					{
						std::ofstream &file = *static_cast<std::ofstream*>(user_data);

						return static_cast<std::size_t>(file.tellp()); // TODO: GET RID OF THIS.
					};

					const ClownLZSS_Callbacks callbacks = {
						.user_data = &out_file,
						.write = WriteCallback,
						.seek = SeekCallback,
						.tell = TellCallback,
					};

					const auto file_buffer = FileToBuffer(in_filename);

					switch (mode->format)
					{
						case Format::CHAMELEON:
							if (moduled)
								return ClownLZSS_ModuledCompressionWrapper(file_buffer.data(), file_buffer.size(), &callbacks, ClownLZSS_ChameleonCompress, module_size, 1);
							else
								return ClownLZSS_ChameleonCompress(file_buffer.data(), file_buffer.size(), &callbacks);

						case Format::COMPER:
							if (moduled)
								;//return ClownLZSS_ModuledCompressionWrapper(file_buffer.data(), file_buffer.size(), &callbacks, ClownLZSS_ComperCompress, module_size, 1);
							else
								return ClownLZSS::ComperCompress(file_buffer.data(), file_buffer.size(), out_file);

						case Format::FAXMAN:
							if (moduled)
								return ClownLZSS_ModuledCompressionWrapper(file_buffer.data(), file_buffer.size(), &callbacks, ClownLZSS_FaxmanCompress, module_size, 1);
							else
								return ClownLZSS_FaxmanCompress(file_buffer.data(), file_buffer.size(), &callbacks);

						case Format::KOSINSKI:
							if (moduled)
								return ClownLZSS_ModuledCompressionWrapper(file_buffer.data(), file_buffer.size(), &callbacks, ClownLZSS_KosinskiCompress, module_size, 0x10);
							else
								return ClownLZSS_KosinskiCompress(file_buffer.data(), file_buffer.size(), &callbacks);

						case Format::KOSINSKIPLUS:
							if (moduled)
								return ClownLZSS_ModuledCompressionWrapper(file_buffer.data(), file_buffer.size(), &callbacks, ClownLZSS_KosinskiPlusCompress, module_size, 1);
							else
								return ClownLZSS_KosinskiPlusCompress(file_buffer.data(), file_buffer.size(), &callbacks);

						case Format::RAGE:
							if (moduled)
								return ClownLZSS_ModuledCompressionWrapper(file_buffer.data(), file_buffer.size(), &callbacks, ClownLZSS_RageCompress, module_size, 1);
							else
								return ClownLZSS_RageCompress(file_buffer.data(), file_buffer.size(), &callbacks);

						case Format::ROCKET:
							if (moduled)
								return ClownLZSS_ModuledCompressionWrapper(file_buffer.data(), file_buffer.size(), &callbacks, ClownLZSS_RocketCompress, module_size, 1);
							else
								return ClownLZSS_RocketCompress(file_buffer.data(), file_buffer.size(), &callbacks);

						case Format::SAXMAN:
							if (moduled)
								;//return ClownLZSS_ModuledCompressionWrapper(file_buffer.data(), file_buffer.size(), &callbacks, ClownLZSS_SaxmanCompressWithHeader, module_size, 1);
							else
								return ClownLZSS::SaxmanCompressWithHeader(file_buffer.data(), file_buffer.size(), out_file);

						case Format::SAXMAN_NO_HEADER:
							if (moduled)
								;//return ClownLZSS_ModuledCompressionWrapper(file_buffer.data(), file_buffer.size(), &callbacks, ClownLZSS_SaxmanCompressWithoutHeader, module_size, 1);
							else
								return ClownLZSS::SaxmanCompressWithoutHeader(file_buffer.data(), file_buffer.size(), out_file);
					}

					return false;
				}();

				if (!success)
				{
					exit_code = EXIT_FAILURE;
					std::cerr << "Error: File could not be compressed\n";
				}
			}
		}
	}

	return exit_code;
}
