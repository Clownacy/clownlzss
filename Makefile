CFLAGS := -O2 -std=c11 -s -Wall -Wextra -pedantic -fno-ident -flto

all: tool

tool: main.c memory_stream.c chameleon.c comper.c kosinski.c kosinskiplus.c moduled.c rocket.c saxman.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
