all: registration.c
	g++ -g -o registration -lcv -lcvaux -lhighgui -I /usr/include/opencv/ registration.c
	g++ -g -o deformation -lcv -lcvaux -lhighgui -I /usr/include/opencv/ deformation.c

clean:
	rm -rf registration.o registration
	rm -rf deformation.o deformation
