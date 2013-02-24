#!/usr/bin/python           # This is server.py file

import socket               # Import socket module
import markup
from xml.dom.minidom import parseString
from Tkinter import *
from PIL import Image, ImageTk, ImageDraw

server = socket.socket()         # Create a socket object
host = socket.gethostname() # Get local machine name
port = 12345                # Reserve a port for your service.

def connect_client ():
	global client;
	server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	server.bind((host, port))        # Bind to the port
	server.listen(0)                 # Now wait for client connection.
	client, addr = server.accept()     # Establish connection with client.
	print 'Got connection from', addr

def disconnect_client ():
	client.close ()

def send_query_to_client (query):
	xml = markup.page( mode='xml' )
	xml.init( encoding='ISO-8859-2' )
	xml.query.open( )
	xml.type(query)
	xml.query.close( )
	data = str(xml)
	client.send(data)

def receive_reply_from_client1 ():
	while True:
   		xml = client.recv(1024)
		if not xml: break
		f = open('/var/www/imagetransfer/test.xml', 'w')
		f.write(xml)
		f.close()

def receive_reply_from_client ():
	data = client.recv(1024)
	return data

def receive_resolution ():
	data = receive_reply_from_client()
	xml = parseString(data)
	xmlTag = xml.getElementsByTagName('type')[0].toxml()
	xmlData=xmlTag.replace('<type>','').replace('</type>','')
	if (xmlData == '1'):
		xmlTag = xml.getElementsByTagName('x')[0].toxml()
		width=xmlTag.replace('<x>','').replace('</x>','')
		xmlTag = xml.getElementsByTagName('y')[0].toxml()
		height=xmlTag.replace('<y>','').replace('</y>','')
		return (int(width), int(height))

def receive_centroid ():
	data = receive_reply_from_client()
	xml = parseString(data)
	xmlTag = xml.getElementsByTagName('type')[0].toxml()
	t = xmlTag.replace('<type>','').replace('</type>','')
	xmlTag = xml.getElementsByTagName('x')[0].toxml()
	cx=xmlTag.replace('<x>','').replace('</x>','')
	xmlTag = xml.getElementsByTagName('y')[0].toxml()
	cy=xmlTag.replace('<y>','').replace('</y>','')
	return (int(t), int(cx), int(cy))


def start_capture ():
	send_query_to_client("start_capture");
	while 1:
		[t, x, y] = receive_centroid ();
		if t == 2:
			canvas.delete(ALL);

		canvas.create_oval(x -5 , y - 5, x + 5, y + 5, outline="red", 
				            fill="green", width=2)
		canvas.update_idletasks()
	return
	
def disconnect ():
	send_query_to_client("disconnect");
	disconnect_client ()
	main.destroy()
	return

main = Tk()
frame = Frame(main)
frame.pack()
label = Label(frame, text="Remote Monitoring Gadget", fg="red")
label.pack()

connect_client();

send_query_to_client("resolution");
[w, h] = receive_resolution ();

canvas = Canvas(main, width=h, height=h)
#canvas.create_rectangle(0 , 0, w, h, outline="yellow", fill="black", width=5)
canvas.pack()

capture = Button(frame, text="Start Capture",command=start_capture, fg="red")
capture.pack(side=LEFT)

disconnect = Button(frame, text="Disconnect",command=disconnect, fg="red")
disconnect.pack(side=LEFT)

main.mainloop()
