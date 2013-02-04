CFLAGS=-Wall
CFLAGS=-L/usr/include -I/usr/lib -lconio -lncurses

all: jogo_da_velha

#jogo_da_velha: jogo_da_velha.c
#cc -o jogo_da_velha jogo_da_velha.c -I/usr/local/include -lconio -lncurses	

clean:
	rm -f jogo_da_velha
