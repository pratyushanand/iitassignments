all: main.c
	rm -rf build out
	mkdir build out
	g++ -c main.c -I /usr/include/opencv/ -o build/main.o
	g++ -o build/main vibe-background.o build/main.o -lcv -lcvaux -lhighgui -lcxcore

clean:
	rm -rf build out
