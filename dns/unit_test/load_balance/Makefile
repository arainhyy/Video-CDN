TEST=test
OBJS=../../parse.o ../../load_balance.o test_load_balance.o
LDLIB=-lcmocka
FLAGS=-Wno-pointer-to-int-cast

test: $(OBJS) $(DEPEND_SRC)
	gcc $(FLAGS) $(OBJS) $(LDLIB) -o $(TEST)

run: test
	./$(TEST)

clean:
	- rm $(TEST)
	- rm $(OBJS)
