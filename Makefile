all: proxy ns

proxy:
	cd src && make && mv proxy .. && cd ..

ns:
	cd dns && make && mv dns ../nameserver && cd ..

clean:
	- cd src && make clean && cd ..
	- cd dns && make clean && cd ..
	- rm proxy
	- rm nameserver
