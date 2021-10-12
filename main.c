/*
	(C) 2018-2021 Clownacy

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clowncommon.h"

#include "chameleon.h"
#include "comper.h"
#include "faxman.h"
#include "kosinski.h"
#include "kosinskiplus.h"
#include "rage.h"
#include "rocket.h"
#include "saxman.h"

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
		"Clownacy's compression tool thingy\n"
		"\n"
		"Usage: tool [options] [in-filename] [out-filename]"
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
		"                    MODULE_SIZE controls the module size (defaults to 0x1000)\n",
		stdout
	);
}

int main(int argc, char **argv)
{
	int i;

	int exit_code = EXIT_SUCCESS;

	const Mode *mode = NULL;
	const char *in_filename = NULL;
	const char *out_filename = NULL;
	cc_bool moduled = cc_false;
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
			fputs("Error: Input file not specified\n", stderr);
			PrintUsage();
		}
		else if (mode == NULL)
		{
			exit_code = EXIT_FAILURE;
			fputs("Error: Format not specified\n", stderr);
			PrintUsage();
		}
		else
		{
			/* Load input file to buffer */
			FILE *in_file;

			if (out_filename == NULL)
				out_filename = moduled ? mode->moduled_default_filename : mode->normal_default_filename;

			in_file = fopen(in_filename, "rb");

			if (in_file == NULL)
			{
				exit_code = EXIT_FAILURE;
				fputs("Error: Could not open input file\n", stderr);
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
					size_t compressed_size;
					unsigned char *compressed_buffer;

					fread(file_buffer, 1, file_size, in_file);
					fclose(in_file);

					/* Compress data */
					compressed_buffer = NULL;

					switch (mode->format)
					{
						case FORMAT_CHAMELEON:
							if (moduled)
								compressed_buffer = ClownLZSS_ModuledChameleonCompress(file_buffer, file_size, &compressed_size, module_size);
							else
								compressed_buffer = ClownLZSS_ChameleonCompress(file_buffer, file_size, &compressed_size);
							break;

						case FORMAT_COMPER:
							if (moduled)
								compressed_buffer = ClownLZSS_ModuledComperCompress(file_buffer, file_size, &compressed_size, module_size);
							else
								compressed_buffer = ClownLZSS_ComperCompress(file_buffer, file_size, &compressed_size);
							break;

						case FORMAT_FAXMAN:
							if (moduled)
								compressed_buffer = ClownLZSS_ModuledFaxmanCompress(file_buffer, file_size, &compressed_size, module_size);
							else
								compressed_buffer = ClownLZSS_FaxmanCompress(file_buffer, file_size, &compressed_size);
							break;

						case FORMAT_KOSINSKI:
							if (moduled)
								compressed_buffer = ClownLZSS_ModuledKosinskiCompress(file_buffer, file_size, &compressed_size, module_size);
							else
								compressed_buffer = ClownLZSS_KosinskiCompress(file_buffer, file_size, &compressed_size);
							break;

						case FORMAT_KOSINSKIPLUS:
							if (moduled)
								compressed_buffer = ClownLZSS_ModuledKosinskiPlusCompress(file_buffer, file_size, &compressed_size, module_size);
							else
								compressed_buffer = ClownLZSS_KosinskiPlusCompress(file_buffer, file_size, &compressed_size);
							break;

						case FORMAT_RAGE:
							if (moduled)
								compressed_buffer = ClownLZSS_ModuledRageCompress(file_buffer, file_size, &compressed_size, module_size);
							else
								compressed_buffer = ClownLZSS_RageCompress(file_buffer, file_size, &compressed_size);
							break;

						case FORMAT_ROCKET:
							if (moduled)
								compressed_buffer = ClownLZSS_ModuledRocketCompress(file_buffer, file_size, &compressed_size, module_size);
							else
								compressed_buffer = ClownLZSS_RocketCompress(file_buffer, file_size, &compressed_size);
							break;

						case FORMAT_SAXMAN:
							if (moduled)
								compressed_buffer = ClownLZSS_ModuledSaxmanCompress(file_buffer, file_size, &compressed_size, cc_true, module_size);
							else
								compressed_buffer = ClownLZSS_SaxmanCompress(file_buffer, file_size, &compressed_size, cc_true);
							break;

						case FORMAT_SAXMAN_NO_HEADER:
							if (moduled)
								compressed_buffer = ClownLZSS_ModuledSaxmanCompress(file_buffer, file_size, &compressed_size, cc_false, module_size);
							else
								compressed_buffer = ClownLZSS_SaxmanCompress(file_buffer, file_size, &compressed_size, cc_false);
							break;
					}

					free(file_buffer);

					if (compressed_buffer == NULL)
					{
						exit_code = EXIT_FAILURE;
						fputs("Error: File could not be compressed\n", stderr);
					}
					else
					{
						/* Write compressed data to output file */
						FILE *out_file = fopen(out_filename, "wb");

						if (out_file == NULL)
						{
							exit_code = EXIT_FAILURE;
							fputs("Error: Could not open output file\n", stderr);
						}
						else
						{
							fwrite(compressed_buffer, compressed_size, 1, out_file);
							fclose(out_file);
						}

						free(compressed_buffer);
					}
				}
			}
		}
	}

	return exit_code;
}
