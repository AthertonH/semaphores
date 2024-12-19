# CS 370
# Simple make file for project #5B

CFLAGS	= -ggdb -O0 -pthread

traffic:	traffic.c
	gcc $(CFLAGS) -o traffic traffic.c

clean:
	@rm -f traffic