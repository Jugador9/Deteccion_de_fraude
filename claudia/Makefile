default:
	@echo \'make FileProcessor\' para compilar la solucion

FileProcessor: FileProcessor.c logger.c
	gcc -Wall -g -c FileProcessor.c logger.c
	gcc -Wall -g -o FileProcessor FileProcessor.o logger.o -lpthread -lrt

clean:
	rm -f FileProcessor *.o

.SUFFIXES:
.PHONY : clean