CC:=gcc

ers: ERS.o 
	$(CC) -o ers ERS.o
ERS.o:
	$(CC) -c ERS.c 

clean:
	rm -f ERS.o 