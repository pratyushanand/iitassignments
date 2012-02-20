all:
	g++ -g -o encoding -lcv -lcvaux -lhighgui -I /usr/include/opencv/ encoding.c
	g++ -g -o decoding -lcv -lcvaux -lhighgui -I /usr/include/opencv/ decoding.c
	g++ -g -o video_encoding -lcv -lcvaux -lhighgui -I /usr/include/opencv/ video_encoding.c
	g++ -g -o video_decoding -lcv -lcvaux -lhighgui -I /usr/include/opencv/ video_decoding.c

clean:
	rm -rf *.o encoding decoding video_encoding video_decoding
