proxy:
	cd src && make && mv proxy .. && cd ..

clean:
	- cd src && make clean && cd ..
	- rm proxy

