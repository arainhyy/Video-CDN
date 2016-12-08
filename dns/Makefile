CC=gcc
OBJ=nameserver.o parse.o log.o load_balance.o \
    ../src/dns_record.o
FLAGS = -g -Wall
EXC=dns
LIB=-I/usr/local/include -L/usr/lib/x86_64-linux-gnu

default: $(EXC)

$(EXC): $(OBJ)
	$(CC) $(FLAGS) $(OBJ) -o $(EXC)

clean:
	- rm -f *~ *.o
	- rm $(EXC)

