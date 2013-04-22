#!/usr/bin/python           # This is server.py file

import socket, markup, time, thread, threading
from xml.dom.minidom import parseString
from Tkinter import *
from PIL import Image, ImageTk, ImageDraw
import base64
import array, io


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

data_array = ''
def receive_reply_from_client ():
	global data_array
	data_array = ''
	xmlType = '2'
	while (xmlType == '2'):
		data = client.recv(40960)
		xml = parseString(str(data))
		xmlTag = xml.getElementsByTagName('type')[0].toxml()
		xmlType=xmlTag.replace('<type>','').replace('</type>','')
		xmlTag = xml.getElementsByTagName('data')[0].toxml()
		xmlData=xmlTag.replace('<data>','').replace('</data>','')
		xml_data = base64.decodestring(xmlData)
		data_array += xml_data
		if (xmlType != '1'):
			send_cmd_to_client("ack");
	return data_array

def receive_resolution ():
	resol = receive_reply_from_client()
	aresol =  array.array('B', resol)
	resx = int(aresol[0]) + int(aresol[1]) * 256
	resy = int(aresol[2]) + int(aresol[3]) * 256
	return (resx, resy)

def draw_rawdata (data):
	print len(data)
#	img = Image.frombuffer('L', (w,h), data, "raw", 'L', 0, 1)
#	img = ImageTk.PhotoImage(img)
#	canvas.create_image(0, 0, image=img, anchor=NW)
#	canvas.pack()
	return


def start_cam_task ():
	global mode_image
	while 1:
		data = receive_reply_from_client()
		if (mode_image.get() == 1) :
			draw_rawdata (data);
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
#img1 = ImageTk.PhotoImage(file="abc.png")
#canvas.delete(ALL);
#canvas.create_image(0, 0, image=img1, anchor=NW)
#canvas.pack()

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
