all: scanner.exe

scanner: scanner.c scanner-driver.c
	gcc -Wall scanner.c scanner-driver.c -o scanner

scanner.exe: scanner.c scanner-driver.c
	gcc -Wall scanner.c scanner-driver.c -o scanner.exe

clean:
	rm -f scanner
