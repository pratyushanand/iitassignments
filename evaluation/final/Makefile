all: main.c
	rm -rf build out
	mkdir build out
	g++ -c -g main.c -I /usr/include/opencv/ -o build/main.o
	g++ -g -o build/main build/main.o -lcv -lcvaux -lhighgui -lcxcore

rasp: main.c
	rm -rf build out
	mkdir build out
	g++ -c -g main.c -I /usr/include/opencv/ -o build/main.o
	g++ -g -o build/main build/main.o -lopencv_core -lopencv_highgui -lopencv_imgproc

clean:
	rm -rf build out
