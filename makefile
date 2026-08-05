CC = gcc

all:
	$(CC) main.c ffmpeg_ops.c -lpthread

clean:
	rm -f a.out
