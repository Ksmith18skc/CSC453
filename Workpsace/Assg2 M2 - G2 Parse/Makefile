all: compile

compile: driver.c parser.c scanner.c symtab.c
	gcc -Wall driver.c parser.c scanner.c symtab.c -o compile

clean:
	rm -f *.o compile
