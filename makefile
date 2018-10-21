OPTS=-std=gnu99 --pedantic -Wall -Werror -pthread -Iinclude -g
TARGETS = rafiki gopher zazu

all: $(TARGETS)

rafiki: rafiki.c shared.o
	gcc $(OPTS) rafiki.c shared.o -Llib -la4 -o rafiki
	
gopher:gopher.c shared.o
	gcc $(OPTS) gopher.c shared.o -Llib -la4 -o gopher
	
zazu: zazu.c shared.o
	gcc $(OPTS) zazu.c shared.o -Llib -la4 -o zazu
	
shared.o: shared.c shared.h
	gcc $(OPTS) -c shared.c -o shared.o
	
clean:
	rm -f *.o rafiki gopher zazu

# export LD_LIBRARY_PATH=~/workspace/AusterityNetwork/lib