
all: mycp cpcp

mycp:
	gcc -Wall mycp.c -o mycp 

cpcp:
	gcc -Wall cpcp.c -o cpcp -lpthread

.PHONY: clean
clean:
	rm -f cpcp mycp
