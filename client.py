#!/usr/bin/python           	# This is client.py file

import struct
import markup
from xml.dom.minidom import parseString



def extract_query_from_xml (pargs):
	xml = parseString(pargs)
	xmlTag = xml.getElementsByTagName('type')[0].toxml()
	xmlData=xmlTag.replace('<type>','').replace('</type>','')
	query = struct.pack("20s", str(xmlData))
	return query

def encode_reply_to_xml (pargs):
	type, x, y = struct.unpack("iii", pargs)
	xml = markup.page( mode='xml' )
	xml.init( encoding='ISO-8859-2' )
	xml.reply.open( )
	xml.type(type)
	xml.x(x)
	xml.y(y)
	xml.reply.close( )
	reply = struct.pack("256s", str(xml))
	return reply
