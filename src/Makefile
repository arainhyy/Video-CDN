CC=gcc
CFLAGS=-I.
DEPS = parse.h y.tab.h
OBJ = y.tab.o lex.yy.o log.o \
      proxy2.o proxy_main.o helper.o \
      parse.o bitrate.o \
      mydns.o dns_record.o\
      #proxy.o
FLAGS = -g -Wall

LIB=-I/usr/local/include -L/usr/lib/x86_64-linux-gnu

default:all

all: proxy ns

lex.yy.c: lexer.l
	flex $^

y.tab.c: parser.y
	yacc -d $^

%.o: %.c $(DEPS)
	$(CC) $(FLAGS) -c -o $@ $< $(CFLAGS)

proxy: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

ns:
	cd dns && make && cp dns ../nameserver && cd ..

clean:
	- rm -f *~ *.o proxy lex.yy.c y.tab.c y.tab.h log
	- cd dns && make clean && cd ..
