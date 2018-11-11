CC = gcc

INCLUDES = conv.h tsc.h

libasound_module_pcm_myplug.so: myplug.c
	$(CC) -Wall -ggdb -o $@ -shared -fPIC -DPIC $< -lasound

conv_sndfile: conv_sndfile.c $(INCLUDES)
	$(CC) -Wall -ggdb -o $@ $< -lsndfile -lfftw3 -lm

all: conv_sndfile

clean:
	$(RM) conv_sndfile *.so
