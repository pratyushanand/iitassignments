#!/usr/bin/python           # This is server.py file

import socket, markup, time, thread, threading
from xml.dom.minidom import parseString
from Tkinter import *
from PIL import Image, ImageTk, ImageDraw
import base64
import array

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)         # Create a socket object
host = socket.gethostname() # Get local machine name
port = 12345                # Reserve a port for your service.
cam_started = 0


def connect_client ():
	global client;
	server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	server.bind((host, port))        # Bind to the port
	server.listen(0)                 # Now wait for client connection.
	client, addr = server.accept()     # Establish connection with client.
	print 'Got connection from', addr

def disconnect_client ():
	print "closing"
	client.close ()

def send_cmd_to_client (cmd):
	xml = markup.page( mode='xml' )
	xml.init( encoding='ISO-8859-2' )
	xml.cmd.open( )
	xml.type(cmd)
	xml.cmd.close( )
	client.send(str(xml))
	return

def receive_reply_from_client ():
	data = client.recv(40960)
	return data

def receive_resolution ():
	data = receive_reply_from_client()
	xml = parseString(str(data))
	xmlTag = xml.getElementsByTagName('type')[0].toxml()
	xmlData=xmlTag.replace('<type>','').replace('</type>','')
	if (xmlData == '1'):
		xmlTag = xml.getElementsByTagName('data')[0].toxml()
		data=xmlTag.replace('<data>','').replace('</data>','')
		resol = base64.b16decode(data);
		aresol =  array.array('B', resol)
		resx = int(aresol[0]) + int(aresol[1]) * 256
		resy = int(aresol[2]) + int(aresol[3]) * 256
		return (resx, resy)
	else:
		print "wrong resolution type received"
		return (0, 0)

centroid = []
def draw_centroid (data):
	global centroid
	xml = parseString(str(data))
	xmlTag = xml.getElementsByTagName('type')[0].toxml()
	xmlData=xmlTag.replace('<type>','').replace('</type>','')
	t = xmlTag.replace('<type>','').replace('</type>','')
	xmlTag = xml.getElementsByTagName('data')[0].toxml()
	data=xmlTag.replace('<data>','').replace('</data>','')
	xy = base64.b16decode(data)
	centroid.insert(0, xy)
	if int(t) == 3:
		canvas.delete(ALL);
		for c in centroid[:]:
			axy =  array.array('B', c)
			x = int(axy[0]) + int(axy[1]) * 256
			y = int(axy[2]) + int(axy[3]) * 256
			canvas.create_oval(x -5 , y - 5, x + 5, y + 5, outline="red", 
		        	    fill="green", width=2)
		canvas.update_idletasks()
		centroid = []
	return

contour = []
def draw_contour (data):
	global contour, w, h
	xml = parseString(str(data))
	xmlTag = xml.getElementsByTagName('type')[0].toxml()
	xmlData=xmlTag.replace('<type>','').replace('</type>','')
	t = xmlTag.replace('<type>','').replace('</type>','')
	xmlTag = xml.getElementsByTagName('data')[0].toxml()
	data=xmlTag.replace('<data>','').replace('</data>','')
	xy = base64.b16decode(data);
	contour.insert(0, xy)
	if int(t) == 3:
		canvas.delete(ALL);
		for c in contour[:]:
			axy =  array.array('B', c)
			num_pt = len(axy)
			x = int(axy[0]) + int(axy[1]) * 256
			y = int(axy[2]) + int(axy[3]) * 256
			i = 4
			while (i < num_pt):
				x1 = int(axy[i + 0]) + int(axy[i + 1]) * 256
				y1 = int(axy[i + 2]) + int(axy[i + 3]) * 256
				canvas.create_line(x , y, x1, y1, fill="green")
				x = x1
				y = y1
				i = i + 4
		canvas.update_idletasks()
		contour = []
	return



def start_cam_task ():
	global mode_image
	while 1:
		data = receive_reply_from_client()
		if (mode_image.get() == 1) :
			draw_centroid (data);
		if (mode_image.get() == 2) :
			draw_contour (data);
		send_cmd_to_client("ack");
	return


def start_cam ():
	global cam_started
	if (cam_started == 0) :
		thread.start_new_thread(start_cam_task, ())
		cam_started = 1
	send_cmd_to_client("start_cam");
	return
		
def stop_cam ():
	send_cmd_to_client("stop_cam");
	return

def disconnect ():
	send_cmd_to_client("disconnect");
	disconnect_client ()
	main.destroy()
	return

def select_image_mode ():
	global mode_image
	if (mode_image.get() == 1) :
		send_cmd_to_client("image_mode_1");
	if (mode_image.get() == 2) :
		send_cmd_to_client("image_mode_2");
	return

connect_client();
send_cmd_to_client("resolution");
[w, h] = receive_resolution ();

main = Tk()
frame = Frame(main, width=w, height=h)
frame.pack()
label = Label(frame, text="Remote Monitoring Gadget")
label.pack()



canvas = Canvas(main, width=w, height=h, bg="black")
canvas.pack()

capture = Button(frame, text="Start Cam",command=start_cam, fg="red")
capture.pack(side=LEFT, padx=2, pady=2)

capture = Button(frame, text="Stop Cam",command=stop_cam, fg="red")
capture.pack(side=LEFT, padx=6, pady=2)

disconnect = Button(frame, text="Disconnect",command=disconnect,
		fg="red")
disconnect.pack(side=LEFT, padx=10, pady=2)

mode_image = IntVar()
mode_image.set(1)
centroid_mode = Radiobutton(frame, text="Enable Centroid mode",
		command=select_image_mode, variable=mode_image,
		value=1)
centroid_mode.pack(anchor=W)
contour_mode = Radiobutton(frame, text="Enable Contour mode",
		command=select_image_mode, variable=mode_image,
		value=2)
contour_mode.pack(anchor=W)

main.mainloop()
