all: main.cpp
	g++ -c -g main.cpp -I /usr/include/opencv/ -I /usr/include/python2.7
	g++ -o main vibe-background.o main.o `pkg-config opencv --cflags --libs` `pkg-config python --cflags --libs`

clean:
	rm -rf *.pyc main.o main
