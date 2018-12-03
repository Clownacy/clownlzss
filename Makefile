CFLAGS := -O2 -s -Wall -Wextra -fno-ident -flto

all: kos kosp sax comp

kos: maink.c kosinski.c memory_stream.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

kosp: mainkp.c kosinskiplus.c memory_stream.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

sax: mains.c saxman.c memory_stream.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

comp: main.c comper.c memory_stream.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
