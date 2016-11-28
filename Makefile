CC=gcc
CFLAGS=-I.
DEPS = parse.h y.tab.h
OBJ = y.tab.o lex.yy.o log.o \
      proxy2.o proxy_main.o helper.o \
      parse.o bitrate.o
      #proxy.o
FLAGS = -g -Wall

default:all

all: proxy

lex.yy.c: lexer.l
	flex $^

y.tab.c: parser.y
	yacc -d $^

%.o: %.c $(DEPS)
	$(CC) $(FLAGS) -c -o $@ $< $(CFLAGS)

proxy: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *~ *.o proxy lex.yy.c y.tab.c y.tab.h log
