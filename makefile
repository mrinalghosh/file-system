all: fs.o

main: fs.o disk.o
	gcc -o main -Werror -Wall -g $^ main.c

fs: fs.o disk.o
	gcc -o $@ -Werror -Wall -g $^

%.o: %.c
	gcc -o $@ -Werror -Wall -g -c $<

clean:
	rm -f fs *.o *~

