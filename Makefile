#NAME: TEJASVI KASTURI
#UID: 604778994
#EMAIL: KASTURITEJASVI@GMAIL.COM

default:
	make build

build: lab4c_tcp.c lab4c_tls.c
	gcc -Wall -Wextra -lm -lmraa -o lab4c_tcp lab4c_tcp.c
	gcc -Wall -Wextra -lm -lmraa -o lab4c_tls lab4c_tls.c -lssl -lcrypto

dist: lab4c_tls.c lab4c_tcp.c Makefile README
	tar -cvzf lab4c-604778994.tar.gz lab4c_tls.c lab4c_tcp.c Makefile README

check: 
	make
	make clean

clean: 
	rm -f lab4c-604778994.tar.gz lab4c_tls lab4c_tcp
