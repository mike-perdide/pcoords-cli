CC=gcc
#CFLAGS=-g -I../ -I../libpicviz/include -I../libpicviz/parser/
CFLAGS=-g `pkg-config libpicviz --cflags`
LIBS=`pkg-config libpicviz --libs` -lpcre

all: pcv

pcv: pcv.c
	@echo $(CC) pcv.c -o pcv $(CFLAGS) $(LIBS)
	$(CC) pcv.c -o pcv $(CFLAGS) $(LIBS)

install:
	install pcv /usr/local/bin

clean:
	rm -f pcv

