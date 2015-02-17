LIBS=	-ladf
SOURCES=error.c misc.c version.c zfile.c
OBJS=	$(SOURCES:.c=.o)
PROGS=	adfcopy adfcreate adfdelete adfdump adfextract adfinfo adfinstall adflist adfmakedir
CC=	gcc
CFLAGS=	-Wall -ggdb

all:	$(PROGS)

adfcopy: $(OBJS) adfcopy.c
	$(CC) $(CFLAGS) -o $@ $(LIBS) $(OBJS) $@.c

adfcreate: $(OBJS) adfcreate.c
	$(CC) $(CFLAGS) -o $@ $(LIBS) $(OBJS) $@.c

adfdelete: $(OBJS) adfdelete.c
	$(CC) $(CFLAGS) -o $@ $(LIBS) $(OBJS) $@.c

adfdump: $(OBJS) adfdump.c
	$(CC) $(CFLAGS) -o $@ $(LIBS) $(OBJS) $@.c

adfextract: $(OBJS) adfextract.c
	$(CC) $(CFLAGS) -o $@ $(LIBS) $(OBJS) $@.c

adfinfo: $(OBJS) adfinfo.c
	$(CC) $(CFLAGS) -o $@ $(LIBS) $(OBJS) $@.c

adfinstall: $(OBJS) adfinstall.c bootblocks.o
	$(CC) $(CFLAGS) -o $@ $(LIBS) $(OBJS) bootblocks.o $@.c

adflist: $(OBJS) adflist.c
	$(CC) $(CFLAGS) -o $@ $(LIBS) $(OBJS) $@.c

adfmakedir: $(OBJS) adfmakedir.c
	$(CC) $(CFLAGS) -o $@ $(LIBS) $(OBJS) $@.c

bootblocks:
	$(CC) $(CFLAGS) -c -o $@.o $@.c

clean:
	rm -f $(PROGS) *.o *~ core *.bb
