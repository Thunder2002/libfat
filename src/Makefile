all: main.exe

main.exe: main.o io.o
	gcc -o main.exe main.o io.o

io.o: io.c
	gcc -c io.c

main.o: main.c
	gcc -c main.c
     
clean:
	del main.o io.o main.exe