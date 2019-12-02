all: main

main: main.cpp 
	mpic++ main.cpp -o main.exe -std=c++11 -pthread
clean: 
	rm main.exe