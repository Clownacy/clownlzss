#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chameleon.h"
#include "comper.h"
#include "kosinski.h"
#include "kosinskiplus.h"
#include "rocket.h"
#include "saxman.h"

typedef enum ModeID
{
	MODE_CHAMELEON,
	MODE_COMPER,
	MODE_KOSINSKI,
	MODE_KOSINSKIPLUS,
	MODE_ROCKET,
	MODE_SAXMAN
} ModeID;

typedef struct Mode
{
	ModeID id;
	char *command;
	char *default_filename;
} Mode;

Mode modes[] = {
	{MODE_CHAMELEON, "-ch", "out.cham"},
	{MODE_COMPER, "-c", "out.comp"},
	{MODE_KOSINSKI, "-k", "out.kos"},
	{MODE_KOSINSKIPLUS, "-kp", "out.kosp"},
	{MODE_ROCKET, "-r", "out.rock"},
	{MODE_SAXMAN, "-s", "out.sax"},
};

int main(int argc, char *argv[])
{
	--argc;
	++argv;

	Mode *mode = NULL;
	char *in_filename = NULL;
	char *out_filename = NULL;

	for (int i = 0; i < argc; ++i)
	{
		if (argv[i][0] == '-')
		{
			for (size_t j = 0; j < sizeof(modes) / sizeof(modes[0]); ++j)
			{
				if (!strcmp(argv[i], modes[j].command))
				{
					mode = &modes[j];
					break;
				}
			}
		}
		else
		{
			if (!in_filename)
				in_filename = argv[i];
			else
				out_filename = argv[i];
		}
	}

	if (!in_filename)
	{
		printf("Error: In-file not specified\n");
	}
	else if (!mode)
	{
		printf("Error: Mode not specified\n");
	}
	else
	{
		if (!out_filename)
			out_filename = mode->default_filename;

		FILE *in_file = fopen(in_filename, "rb");

		if (in_file)
		{
			fseek(in_file, 0, SEEK_END);
			const size_t file_size = ftell(in_file);
			rewind(in_file);

			unsigned char *file_buffer = malloc(file_size);
			fread(file_buffer, 1, file_size, in_file);
			fclose(in_file);

			size_t compressed_size;
			unsigned char *compressed_buffer;

			switch (mode->id)
			{
				case MODE_CHAMELEON:
					compressed_buffer = ChameleonCompress(file_buffer, file_size, &compressed_size);
					break;
				case MODE_COMPER:
					compressed_buffer = ComperCompress(file_buffer, file_size, &compressed_size);
					break;
				case MODE_KOSINSKI:
					compressed_buffer = KosinskiCompress(file_buffer, file_size, &compressed_size);
					break;
				case MODE_KOSINSKIPLUS:
					compressed_buffer = KosinskiPlusCompress(file_buffer, file_size, &compressed_size);
					break;
				case MODE_ROCKET:
					compressed_buffer = RocketCompress(file_buffer, file_size, &compressed_size);
					break;
				case MODE_SAXMAN:
					compressed_buffer = SaxmanCompress(file_buffer, file_size, &compressed_size);
					break;
				default:	// Impossible but GCC won't shut up about it
					return 0;
			}

			FILE *out_file = fopen(out_filename, "wb");

			if (out_file)
			{
				if (mode->id == MODE_ROCKET)
				{
					fputc(file_size >> 8, out_file);
					fputc(file_size & 0xFF, out_file);
					fputc(compressed_size >> 8, out_file);
					fputc(compressed_size & 0xFF, out_file);
				}
				else if (mode->id == MODE_SAXMAN)
				{
					fputc(compressed_size & 0xFF, out_file);
					fputc(compressed_size >> 8, out_file);
				}

				fwrite(compressed_buffer, compressed_size, 1, out_file);
				free(compressed_buffer);
				fclose(out_file);
			}
		}
	}
}
