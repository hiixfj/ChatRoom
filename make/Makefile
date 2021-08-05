all: server client

.PHONY:all

VPATH = ./..

objects = server.o ser.o group.o func.o mysql.o

server : $(objects)
	cc -o server $(objects) -pthread -lmysqlclient

client : client.o func.o
	cc -o client client.o func.o -pthread -lmysqlclient 


server.o : func.h

ser.o : func.h

group.o : func.h

func.o : func.h

mysql.o : func.h


client.o : func.h



.PHONY : clean
clean : 
	rm server client client.o $(objects) 