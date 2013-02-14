CFLAGS=-Wall
#CFLAGS=-L/usr/include -I/usr/lib -lconio -lncurses

all: jogo_da_velha

messenger.o: messenger.c

jogo_da_velha.o: jogo_da_velha.c

jogo_da_velha: jogo_da_velha.o messenger.o

#jogo_da_velha: jogo_da_velha.c
#cc -o jogo_da_velha jogo_da_velha.c -I/usr/local/include -lconio -lncurses	

clean:
	rm -f *.o jogo_da_velha
