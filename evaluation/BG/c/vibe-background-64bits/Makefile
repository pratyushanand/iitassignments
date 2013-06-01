all: main.c
	rm -rf build out
	mkdir build out
	g++ -O3 -o build/main main.c `pkg-config opencv --cflags --libs`

clean:
	rm -rf build out
