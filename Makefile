all: server.out   client.out

server.out: server.o jsoncpp.o  
	g++ -std=c++11 -o server.out server.o jsoncpp.o

client.out: client.o jsoncpp.o  
	g++ -std=c++11 -o client.out client.o jsoncpp.o

client.o: client.cpp def.hpp json/json.h
	g++ -std=c++11 -c client.cpp

server.o: server.cpp Users/* def.hpp Exceptions/* json/json.h
	g++ -std=c++11 -c server.cpp

jsoncpp.o: jsoncpp.cpp json/json.h
	g++ -std=c++11 -c jsoncpp.cpp

clean:
	rm *.o
	rm *.out
