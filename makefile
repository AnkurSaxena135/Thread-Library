mythread.a: threadlib.o
	ar rcs mythread.a threadlib.o

thread: threadlib.c mythread.h 
	gcc -w -o threadlib.o -c threadlib.c

clean:
	rm threadlib.o 
