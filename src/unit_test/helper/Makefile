TEST=test
OBJS=test_helper.o ../../helper.o
LDLIB=-lcmocka -I/usr/local/include -L/usr/local/lib
FLAGS=-Wno-pointer-to-int-cast
CC=gcc
# try 'export C_INCLUDE_PATH=/usr/local/include:/usr/local/Cellar/libxml2/2.9.4/include/libxml2' if -I doesn't work

test: $(OBJS) $(DEPEND_SRC)
	$(CC) $(FLAGS) $(OBJS) $(LDLIB) -o $(TEST)

run: test
	./$(TEST)

clean:
	- rm $(TEST)
	- rm $(OBJS)
