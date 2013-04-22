all: main.cpp
	rm -rf build out
	mkdir build out
	g++ main.cpp -o ./build/main `pkg-config opencv --cflags --libs` `pkg-config python --cflags --libs`

clean:
	rm -rf build out
