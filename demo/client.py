#!/usr/bin/python           	# This is client.py file

import struct
import markup
import array
import base64
from xml.dom.minidom import parseString



def extract_cmd_from_xml (pargs):
	xml = parseString(pargs)
	xmlTag = xml.getElementsByTagName('type')[0].toxml()
	xmlData=xmlTag.replace('<type>','').replace('</type>','')
	cmd = struct.pack("20s", str(xmlData))
	return cmd

def encode_reply_to_xml (pargs):
	data =  array.array('B', pargs)
	xml = markup.page( mode='xml' )
	xml.init( encoding='ISO-8859-2' )
	xml.reply.open( )
	xml.type(data[0])
	length = int(data[1]) + (int(data[2]) << 8) + (int(data[3]) << 16)
	xml.data(base64.encodestring(data[4:].tostring()))
	xml.reply.close( )
	reply = struct.pack("40960s", str(xml))
	print len(reply)
	return reply
