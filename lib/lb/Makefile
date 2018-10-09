.PHONY=all

FLAGS=-g -std=c99 -pedantic -Wall -Werror -fPIC


all: liba4.so

liba4.so: player.o types.o smsg.o cmsg.o hub.o utils.o
	gcc -shared -Wl,-soname,liba4.so.1 player.o types.o smsg.o cmsg.o hub.o utils.o -o liba4.so.1.0

utils.o: utils.c utils.h types.h
	gcc $(FLAGS) -c utils.c

player.o: player.c player.h types.h cmsg.h
	gcc $(FLAGS) -c player.c

cmsg.o: cmsg.c cmsg.h utils.h types.h
	gcc $(FLAGS) -c cmsg.c

smsg.o: smsg.c smsg.h
	gcc $(FLAGS) -c smsg.c

types.o: types.c types.h
	gcc $(FLAGS) -c types.c
	
hub.o: hub.c types.h
	gcc $(FLAGS) -c hub.c
clean:
	rm *.o
