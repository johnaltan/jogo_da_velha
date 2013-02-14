
CFLAGS=-c -Wall -Ideps/libuv/include -pthread -D_REENTRANT
LDLIBS=-lpthread -lrt

all: jogo

deps:
	mkdir deps

deps/libuv: deps
	cd deps;git clone https://github.com/joyent/libuv.git

deps/libuv/libuv.a: deps/libuv
	cd deps/libuv;make

jogo.o: jogo.c deps/libuv

jogo: jogo.o deps/libuv/libuv.a

test: jogo
	bash -c 'ulimit -m 40000;./jogo --port 8000 --masters 127.0.0.1:8000,127.0.0.1:8001 --setauth bla'

testb: jogo
	bash -c 'ulimit -m 40000;./jogo --port 8001 --masters 127.0.0.1:8000,127.0.0.1:8001'

test2: jogo
	sh -c 'ulimit -m 40000;./jogo --port 8000 --masters 127.0.0.1:8000,127.0.0.1:8001 &'
	sh -c 'ulimit -m 40000;./jogo --port 8001 --masters 127.0.0.1:8000,127.0.0.1:8001 &'

clean:
	rm -f jogo *.o

