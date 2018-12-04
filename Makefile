CFLAGS := -O2 -s -Wall -Wextra -fno-ident -flto -include stb_leakcheck.h

all: kos kosp sax comp rock cham

kos: main.c kosinski.c memory_stream.c
	$(CC) $(CFLAGS) -DKOSINSKI -o $@ $^ $(LDFLAGS) $(LIBS)

kosp: main.c kosinskiplus.c memory_stream.c
	$(CC) $(CFLAGS) -DKOSINSKIPLUS -o $@ $^ $(LDFLAGS) $(LIBS)

sax: main.c saxman.c memory_stream.c
	$(CC) $(CFLAGS) -DSAXMAN -o $@ $^ $(LDFLAGS) $(LIBS)

comp: main.c comper.c memory_stream.c
	$(CC) $(CFLAGS) -DCOMPER -o $@ $^ $(LDFLAGS) $(LIBS)

rock: main.c rocket.c memory_stream.c
	$(CC) $(CFLAGS) -DROCKET -o $@ $^ $(LDFLAGS) $(LIBS)

cham: main.c chameleon.c memory_stream.c
	$(CC) $(CFLAGS) -DCHAMELEON -o $@ $^ $(LDFLAGS) $(LIBS)
