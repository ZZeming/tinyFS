all: tinyFsDemo

tinyFsDemo: tinyFsDemo.o libTinyFS.o libDisk.o linkedList.o libTinyFS.h libDisk.h linkedList.h errorCode.h
	gcc -I -Wall -ggdb -o tinyFsDemo tinyFsDemo.o libTinyFS.o libDisk.o linkedList.o libTinyFS.h libDisk.h linkedList.h errorCode.h

tinyFsDemo.o: tinyFsDemo.c
	gcc -Wall -ggdb -c -o tinyFsDemo.o tinyFsDemo.c

linkedList.o: linkedList.c linkedList.h
	gcc -Wall -ggdb -c -o linkedList.o linkedList.c

libTinyFS.o: libTinyFS.c libTinyFS.h
	gcc -Wall -ggdb -c -o libTinyFS.o libTinyFS.c

libDisk.o: libDisk.c libDisk.h
	gcc -Wall -ggdb -c -o libDisk.o libDisk.c


grind: tinyFsDemo
	valgrind --leak-check=yes -s ./tinyFsDemo

clean: rm *.o