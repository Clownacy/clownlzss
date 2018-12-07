#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chameleon.h"
#include "comper.h"
#include "kosinski.h"
#include "kosinskiplus.h"
#include "moduled.h"
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
	char *default_filename_moduled;
	size_t module_alignment;
	unsigned char* (*function)(unsigned char *data, size_t data_size, size_t *compressed_size);

} Mode;

Mode modes[] = {
	{MODE_CHAMELEON, "-ch", "out.cham", "out.chamm", 1, ChameleonCompress},
	{MODE_COMPER, "-c", "out.comp", "out.compm", 1, ComperCompress},
	{MODE_KOSINSKI, "-k", "out.kos", "out.kosm", 0x10, KosinskiCompress},
	{MODE_KOSINSKIPLUS, "-kp", "out.kosp", "out.kospm", 1, KosinskiPlusCompress},
	{MODE_ROCKET, "-r", "out.rock", "out.rockm", 1, RocketCompress},
	{MODE_SAXMAN, "-s", "out.sax", "out.saxm", 1, SaxmanCompress},
};

int main(int argc, char *argv[])
{
	--argc;
	++argv;

	Mode *mode = NULL;
	char *in_filename = NULL;
	char *out_filename = NULL;
	bool moduled = false;

	for (int i = 0; i < argc; ++i)
	{
		if (argv[i][0] == '-')
		{
			if (!strcmp(argv[i], "-m"))
				moduled = true;

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
			out_filename = moduled ? mode->default_filename_moduled : mode->default_filename;

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

			if (moduled)
				compressed_buffer = ModuledCompress(file_buffer, file_size, &compressed_size, mode->function, 0x1000, mode->module_alignment);
			else
				compressed_buffer = mode->function(file_buffer, file_size, &compressed_size);

			FILE *out_file = fopen(out_filename, "wb");

			if (out_file)
			{
				fwrite(compressed_buffer, compressed_size, 1, out_file);
				free(compressed_buffer);
				fclose(out_file);
			}
		}
	}
}
