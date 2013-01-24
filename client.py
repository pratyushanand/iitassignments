#!/usr/bin/python           	# This is client.py file

import struct
import markup
import socket               	# Import socket module
from xml.dom.minidom import parseString


client = socket.socket()        # Create a socket object
client.settimeout(None)
host = socket.gethostname() 	# Get local machine name
port = 12345                	# Reserve a port for your service.

def connect_to_server ():
	if client.connect((host, port)):
		return 1
	else:
		return 0


def disconnect_from_server ():
	if client.close():
		return 1
	else:
		return 0


def receive_query_from_server ():
	data = client.recv(1024)
	xml = parseString(data)
	xmlTag = xml.getElementsByTagName('type')[0].toxml()
	xmlData=xmlTag.replace('<type>','').replace('</type>','')
	query = struct.pack("20s", str(xmlData))
	return query

def send_reply_to_server (pargs):
	type, x, y = struct.unpack("iii", pargs)
	xml = markup.page( mode='xml' )
	xml.init( encoding='ISO-8859-2' )
	xml.reply.open( )
	xml.type(type)
	xml.x(x)
	xml.y(y)
	xml.reply.close( )
	data = str(xml)
	client.send(data)
	return
