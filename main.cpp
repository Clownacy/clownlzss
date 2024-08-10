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

#include <fstream>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "clowncommon/clowncommon.h"

#include "chameleon.h"
#include "comper.h"
#include "faxman.h"
#include "kosinski.h"
#include "kosinskiplus.h"
#include "rage.h"
#include "rocket.h"
#include "saxman.h"

#include "decompressors/comper.h"
#include "decompressors/kosinski.h"
#include "decompressors/saxman.h"

typedef enum Format
{
	FORMAT_CHAMELEON,
	FORMAT_COMPER,
	FORMAT_FAXMAN,
	FORMAT_KOSINSKI,
	FORMAT_KOSINSKIPLUS,
	FORMAT_RAGE,
	FORMAT_ROCKET,
	FORMAT_SAXMAN,
	FORMAT_SAXMAN_NO_HEADER
} Format;

typedef struct Mode
{
	const char *command;
	Format format;
	const char *normal_default_filename;
	const char *moduled_default_filename;
} Mode;

static const Mode modes[] = {
	{"-ch", FORMAT_CHAMELEON,        "out.cham", "out.chamm"},
	{"-c",  FORMAT_COMPER,           "out.comp", "out.compm"},
	{"-f",  FORMAT_FAXMAN,           "out.fax",  "out.faxm" },
	{"-k",  FORMAT_KOSINSKI,         "out.kos",  "out.kosm" },
	{"-kp", FORMAT_KOSINSKIPLUS,     "out.kosp", "out.kospm"},
	{"-ra", FORMAT_RAGE,             "out.rage", "out.ragem"},
	{"-r",  FORMAT_ROCKET,           "out.rock", "out.rockm"},
	{"-s",  FORMAT_SAXMAN,           "out.sax",  "out.saxm" },
	{"-sn", FORMAT_SAXMAN_NO_HEADER, "out.sax",  "out.saxm" }
};

static void PrintUsage(void)
{
	fputs(
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
		"  -d     Decompress (Saxman only)\n",
		stdout
	);
}

static unsigned char ReadCallback(void* const user_data)
{
	std::fstream* const file = (std::fstream*)user_data;

//	file->seekp(1, file->cur);
	return file->get();
}

static void WriteCallback(void* const user_data, const unsigned char byte)
{
	std::fstream* const file = (std::fstream*)user_data;

//	file->seekg(1, file->cur);
	file->put(byte);
}

static void SeekCallback(void* const user_data, const size_t position)
{
	std::fstream* const file = (std::fstream*)user_data;

	file->seekg(position, file->beg);
	file->seekp(position, file->beg);
}

static size_t TellCallback(void* const user_data)
{
	std::fstream* const file = (std::fstream*)user_data;

	return (size_t)file->tellp();
}

#define CLOWNLZSS_READ_INPUT in_file.get()
#define CLOWNLZSS_WRITE_OUTPUT(VALUE) out_file.put(VALUE)
#define CLOWNLZSS_FILL_OUTPUT(VALUE, COUNT, MAXIMUM_COUNT) \
	do \
	{ \
		unsigned int i; \
		for (i = 0; i < COUNT; ++i) \
			out_file.put(VALUE); \
	} \
	while (0)
#define CLOWNLZSS_COPY_OUTPUT(OFFSET, COUNT, MAXIMUM_COUNT) \
	do \
	{ \
		unsigned int i; \
		char bytes[MAXIMUM_COUNT]; \
\
		const auto position = out_file.tellg(); \
		out_file.seekg(-(long)OFFSET, out_file.cur); \
		out_file.read(bytes, COUNT); \
		out_file.seekg(position); \
\
		for (i = OFFSET; i < COUNT; ++i) \
			bytes[i] = bytes[i - OFFSET]; \
\
		out_file.write(bytes, COUNT); \
	} \
	while (0)

static void ComperDecompress(std::fstream &out_file, std::fstream &in_file)
{
	CLOWNLZSS_COMPER_DECOMPRESS;
}

static void SaxmanDecompress(std::fstream &out_file, std::fstream &in_file)
{
	const unsigned int uncompressed_length_lower_byte = ReadCallback(&in_file);
	const unsigned int uncompressed_length_upper_byte = ReadCallback(&in_file);
	const unsigned int uncompressed_length = uncompressed_length_upper_byte << 8 | uncompressed_length_lower_byte;
	CLOWNLZSS_SAXMAN_DECOMPRESS(uncompressed_length);
}
#undef CLOWNLZSS_READ_INPUT
#undef CLOWNLZSS_WRITE_OUTPUT
#undef CLOWNLZSS_FILL_OUTPUT
#undef CLOWNLZSS_COPY_OUTPUT

static void KosinskiDecompress(std::fstream &out_file, std::fstream &in_file)
{
	ClownLZSS_KosinskiDecompress(in_file, out_file);
}

int main(int argc, char **argv)
{
	int i;

	int exit_code = EXIT_SUCCESS;

	const Mode *mode = NULL;
	const char *in_filename = NULL;
	const char *out_filename = NULL;
	cc_bool moduled = cc_false, decompress = cc_false;
	size_t module_size = 0x1000;

	/* Skip past the executable name */
	--argc;
	++argv;

	/* Parse arguments */
	for (i = 0; i < argc; ++i)
	{
		if (argv[i][0] == '-')
		{
			if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
			{
				PrintUsage();
			}
			else if (!strncmp(argv[i], "-m", 2))
			{
				char *argument = strchr(argv[i], '=');

				moduled = cc_true;

				if (argument != NULL)
				{
					char *end;
					unsigned long result = strtoul(argument + 1, &end, 0);

					if (*end != '\0')
					{
						fputs("Invalid parameter to -m\n", stderr);
						exit_code = EXIT_FAILURE;
						break;
					}
					else
					{
						module_size = result;

						if (module_size > 0x1000)
							fputs("Warning: the moduled format header does not fully support sizes greater than\n 0x1000 - header will likely be invalid!\n", stderr);
					}
				}
			}
			else if (!strcmp(argv[i], "-d"))
			{
				decompress = cc_true;
			}
			else
			{
				size_t j;

				for (j = 0; j < CC_COUNT_OF(modes); ++j)
				{
					if (!strcmp(argv[i], modes[j].command))
					{
						mode = &modes[j];
						break;
					}
				}
			}
		}
		else
		{
			if (in_filename == NULL)
				in_filename = argv[i];
			else
				out_filename = argv[i];
		}
	}

	if (exit_code != EXIT_FAILURE)
	{
		if (in_filename == NULL)
		{
			exit_code = EXIT_FAILURE;
			fputs("Error: Input file not specified\n\n", stderr);
			PrintUsage();
		}
		else if (mode == NULL)
		{
			exit_code = EXIT_FAILURE;
			fputs("Error: Format not specified\n\n", stderr);
			PrintUsage();
		}
		else
		{
			/* Load input file to buffer */
			std::fstream in_file(in_filename, std::fstream::in | std::fstream::binary);

			if (!in_file)
			{
				exit_code = EXIT_FAILURE;
				fputs("Error: Could not open input file\n", stderr);
			}
			else
			{
				if (out_filename == NULL)
					out_filename = moduled ? mode->moduled_default_filename : mode->normal_default_filename;

				/* Write compressed data to output file */
				std::fstream out_file(out_filename, decompress ? std::fstream::trunc | std::fstream::in | std::fstream::out | std::fstream::binary : std::fstream::out | std::fstream::binary);

				if (!out_file)
				{
					exit_code = EXIT_FAILURE;
					fputs("Error: Could not open output file\n", stderr);
				}
				else
				{
					ClownLZSS_Callbacks callbacks;

					callbacks.user_data = &out_file;
					callbacks.read = ReadCallback;
					callbacks.write = WriteCallback;
					callbacks.seek = SeekCallback;
					callbacks.tell = TellCallback;

					if (decompress)
					{
						switch (mode->format)
						{
							case FORMAT_COMPER:
								ComperDecompress(out_file, in_file);
								break;

							case FORMAT_KOSINSKI:
								KosinskiDecompress(out_file, in_file);
								break;

							case FORMAT_SAXMAN:
								SaxmanDecompress(out_file, in_file);
								break;
						}
					}
					else
					{
						size_t file_size;
						unsigned char *file_buffer;

						in_file.seekg(0, in_file.end);
						file_size = in_file.tellg();
						in_file.seekg(0, in_file.beg);

						file_buffer = (unsigned char*)malloc(file_size);

						if (file_buffer == NULL)
						{
							exit_code = EXIT_FAILURE;
							fputs("Error: Could not allocate memory for the file buffer\n", stderr);
						}
						else
						{
							/* Compress data */
							cc_bool success = cc_false;

							in_file.read((char*)file_buffer, file_size);
							in_file.close();

							switch (mode->format)
							{
								case FORMAT_CHAMELEON:
									if (moduled)
										success = ClownLZSS_ModuledCompressionWrapper(file_buffer, file_size, &callbacks, ClownLZSS_ChameleonCompress, module_size, 1);
									else
										success = ClownLZSS_ChameleonCompress(file_buffer, file_size, &callbacks);
									break;

								case FORMAT_COMPER:
									if (moduled)
										success = ClownLZSS_ModuledCompressionWrapper(file_buffer, file_size, &callbacks, ClownLZSS_ComperCompress, module_size, 1);
									else
										success = ClownLZSS_ComperCompress(file_buffer, file_size, &callbacks);
									break;

								case FORMAT_FAXMAN:
									if (moduled)
										success = ClownLZSS_ModuledCompressionWrapper(file_buffer, file_size, &callbacks, ClownLZSS_FaxmanCompress, module_size, 1);
									else
										success = ClownLZSS_FaxmanCompress(file_buffer, file_size, &callbacks);
									break;

								case FORMAT_KOSINSKI:
									if (moduled)
										success = ClownLZSS_ModuledCompressionWrapper(file_buffer, file_size, &callbacks, ClownLZSS_KosinskiCompress, module_size, 0x10);
									else
										success = ClownLZSS_KosinskiCompress(file_buffer, file_size, &callbacks);
									break;

								case FORMAT_KOSINSKIPLUS:
									if (moduled)
										success = ClownLZSS_ModuledCompressionWrapper(file_buffer, file_size, &callbacks, ClownLZSS_KosinskiPlusCompress, module_size, 1);
									else
										success = ClownLZSS_KosinskiPlusCompress(file_buffer, file_size, &callbacks);
									break;

								case FORMAT_RAGE:
									if (moduled)
										success = ClownLZSS_ModuledCompressionWrapper(file_buffer, file_size, &callbacks, ClownLZSS_RageCompress, module_size, 1);
									else
										success = ClownLZSS_RageCompress(file_buffer, file_size, &callbacks);
									break;

								case FORMAT_ROCKET:
									if (moduled)
										success = ClownLZSS_ModuledCompressionWrapper(file_buffer, file_size, &callbacks, ClownLZSS_RocketCompress, module_size, 1);
									else
										success = ClownLZSS_RocketCompress(file_buffer, file_size, &callbacks);
									break;

								case FORMAT_SAXMAN:
									if (moduled)
										success = ClownLZSS_ModuledCompressionWrapper(file_buffer, file_size, &callbacks, ClownLZSS_SaxmanCompressWithHeader, module_size, 1);
									else
										success = ClownLZSS_SaxmanCompressWithHeader(file_buffer, file_size, &callbacks);
									break;

								case FORMAT_SAXMAN_NO_HEADER:
									if (moduled)
										success = ClownLZSS_ModuledCompressionWrapper(file_buffer, file_size, &callbacks, ClownLZSS_SaxmanCompressWithoutHeader, module_size, 1);
									else
										success = ClownLZSS_SaxmanCompressWithoutHeader(file_buffer, file_size, &callbacks);
									break;
							}

							if (!success)
							{
								exit_code = EXIT_FAILURE;
								fputs("Error: File could not be compressed\n", stderr);
							}

							free(file_buffer);
						}
					}
				}
			}
		}
	}

	return exit_code;
}
