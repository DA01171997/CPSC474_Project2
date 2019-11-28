all: main

main: main.cpp 
	mpic++ main.cpp -o main.exe -std=c++11

clean: 
	rm main.exe