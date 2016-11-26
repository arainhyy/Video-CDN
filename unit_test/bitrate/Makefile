TEST=test
OBJS=test_bitrate.o ../../bitrate.o
LDLIB=-lcmocka -I/usr/include/libxml2/ -L/usr/lib/x86_64-linux-gnu -lxml2
FLAGS=-Wno-pointer-to-int-cast

test: $(OBJS) $(DEPEND_SRC)
	gcc $(FLAGS) $(OBJS) $(LDLIB) -o $(TEST)

run: test
	./$(TEST)

clean:
	- rm $(TEST)
	- rm $(OBJS)
