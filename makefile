shell.x: main.o
	gcc -o shell.x main.o
main.o: main.c
	gcc -c  -std=gnu99 main.c
clean:
	rm *.o shell.x