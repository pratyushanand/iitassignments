#!/usr/bin/python           # This is server.py file

import socket               # Import socket module
import markup
from xml.dom.minidom import parseString
import Image, ImageDraw, ImageTk, Tkinter

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
	xmlData=xmlTag.replace('<type>','').replace('</type>','')
	if (xmlData == '2'):
		xmlTag = xml.getElementsByTagName('x')[0].toxml()
		cx=xmlTag.replace('<x>','').replace('</x>','')
		xmlTag = xml.getElementsByTagName('y')[0].toxml()
		cy=xmlTag.replace('<y>','').replace('</y>','')
		return (int(cx), int(cy))


connect_client();

send_query_to_client("resolution");

[width, height] = receive_resolution ();
img = Image.new("RGB", (width, height))
draw = ImageDraw.Draw(img)
root = Tkinter.Tk()
root.geometry('+%d+%d' % (width,height))
old_label_image = None

send_query_to_client("start_capture");

while 1:
	[cx, cy] = receive_centroid ();
	print cx 
	print cy
	draw.ellipse((cx - 5, cy - 5, cx + 5, cy + 5), 200, 100);
	tkpi = ImageTk.PhotoImage(img)
	label_image = Tkinter.Label(root, image=tkpi)
	label_image.place(x=0,y=0,width=img.size[0],height=img.size[1])
	root.title("hello")
	if old_label_image is not None:
		old_label_image.destroy()
	old_label_image = label_image

disconnect_client();
