all: listener.cpp talker.cpp
	g++ -std=c++11 -o listener listener.cpp
	g++ -std=c++11 -o talker talker.cpp
