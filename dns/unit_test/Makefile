dirs = $(shell ls)

tests:
	@$(foreach dir,$(dirs), cd $(dir) && make run && cd ..;)

clean:
	@$(foreach dir,$(dirs), cd $(dir) && make clean && cd ..;)
