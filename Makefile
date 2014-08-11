CC=gcc
AR=ar
LIBS=-logg -lvorbis -lvorbisenc

CFLAGS=-Wall 

default:stable

debug:clean 
	$(CC) $(CFLAGS) -g -c -o sdrvorbisenc_dbg.o sdrvorbisenc.c $(LIBS)
	$(AR) rcs libsdrvorbisenc.a sdrvorbisenc_dbg.o
stable:clean 
	$(CC) $(CFLAGS) -O2 -c -o sdrvorbisenc.o sdrvorbisenc.c $(LIBS)
	$(AR) rcs libsdrvorbisenc.a sdrvorbisenc.o
clean:
	rm -vfr libsdrvorbisenc.a *.o
	
example: default
	$(CC) $(CFLAGS) -o sdrvorbisenc_example  sdrvorbisenc_example.c  -L. -lsdrvorbisenc $(LIBS)
