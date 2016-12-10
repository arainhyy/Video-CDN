all: proxy

proxy:
	cd src && make && mv proxy .. && mv nameserver .. && cd ..

clean:
	- cd src && make clean && cd ..
	- rm proxy
	- rm nameserver
