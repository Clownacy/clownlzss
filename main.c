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

#include <stddef.h>
#include <stdio.h>
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
	FILE* const file = (FILE*)user_data;

	return fgetc(file);
}

static void WriteCallback(void* const user_data, const unsigned char byte)
{
	FILE* const file = (FILE*)user_data;

	fputc(byte, file);
}

static void SeekCallback(void* const user_data, const size_t position)
{
	FILE* const file = (FILE*)user_data;

	fseek(file, (long)position, SEEK_SET);
}

static size_t TellCallback(void* const user_data)
{
	FILE* const file = (FILE*)user_data;

	return (size_t)ftell(file);
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
			FILE *in_file = fopen(in_filename, "rb");

			if (in_file == NULL)
			{
				exit_code = EXIT_FAILURE;
				fputs("Error: Could not open input file\n", stderr);
			}
			else
			{
				FILE *out_file;

				if (out_filename == NULL)
					out_filename = moduled ? mode->moduled_default_filename : mode->normal_default_filename;

				/* Write compressed data to output file */
				out_file = fopen(out_filename, decompress ? "w+b" : "wb");

				if (out_file == NULL)
				{
					exit_code = EXIT_FAILURE;
					fputs("Error: Could not open output file\n", stderr);
				}
				else
				{
					ClownLZSS_Callbacks callbacks;

					callbacks.user_data = out_file;
					callbacks.read = ReadCallback;
					callbacks.write = WriteCallback;
					callbacks.seek = SeekCallback;
					callbacks.tell = TellCallback;

					if (decompress)
					{
						const unsigned int uncompressed_length_lower_byte = fgetc(in_file);
						const unsigned int uncompressed_length_upper_byte = fgetc(in_file);
						const unsigned int uncompressed_length = uncompressed_length_upper_byte << 8 | uncompressed_length_lower_byte;
						ClownLZSS_SaxmanDecompress(ReadCallback, in_file, &callbacks, uncompressed_length);
					}
					else
					{
						size_t file_size;
						unsigned char *file_buffer;

						fseek(in_file, 0, SEEK_END);
						file_size = ftell(in_file);
						rewind(in_file);

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

							fread(file_buffer, 1, file_size, in_file);
							fclose(in_file);
							in_file = NULL;

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

					fclose(out_file);
				}

				if (in_file != NULL)
					fclose(in_file);
			}
		}
	}

	return exit_code;
}
