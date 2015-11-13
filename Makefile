ish : ish.o tools.o
	cc -o ish ish.o tools.o

tools.o : tools.c tools.h
	cc -c tools.c

ish.o : ish.c tools.h
	cc -c ish.c

clean :
	rm -f ish *.o
