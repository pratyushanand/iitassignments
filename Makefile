all:
	gcc -I/usr/include/opencv main.c image_grabber.c image_executer.c -o semantics -lcv -lhighgui -lcvaux -lcxcore

clean:
	rm -rf *.o semantics
