CFLAGS := -O2 -s -Wall -Wextra -fno-ident -flto -include stb_leakcheck.h

all: kos kosp sax comp rock cham

kos: maink.c kosinski.c memory_stream.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

kosp: mainkp.c kosinskiplus.c memory_stream.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

sax: mains.c saxman.c memory_stream.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

comp: main.c comper.c memory_stream.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

rock: mainr.c rocket.c memory_stream.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

cham: mainch.c chameleon.c memory_stream.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
