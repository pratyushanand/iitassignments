all: convert.c
	gcc -g -o convert -lcv -lcvaux -lhighgui -I /usr/include/opencv/ convert.c
	gcc -g -o enhancement -lcv -lcvaux -lhighgui -I /usr/include/opencv/ enhancement.c
	gcc -g -o edge_detect -lcv -lcvaux -lhighgui -I /usr/include/opencv/ edge_detect.c
	gcc -g -o edge_link -lcv -lcvaux -lhighgui -I /usr/include/opencv/ edge_link.c
	gcc -g -o halftone -lcv -lcvaux -lhighgui -I /usr/include/opencv/ halftone.c
	gcc -g -o thickening -lcv -lcvaux -lhighgui -I /usr/include/opencv/ thickening.c
	gcc -g -o cleaning -lcv -lcvaux -lhighgui -I /usr/include/opencv/ cleaning.c
	gcc -g -o superposition -lcv -lcvaux -lhighgui -I /usr/include/opencv/ superposition.c

clean:
	rm -rf *.o convert enhancement edge_detect edge_link halftone thickening cleaning superposition
