#!/usr/bin/python
import socket, sys
from dns_common import sendDNSQuery

UDP_IP = '127.0.0.1' #'5.0.0.1'
UDP_PORT = 5353
LOCAL_IP = '127.0.0.1' #'1.0.0.1'

print sendDNSQuery(sys.argv[1],LOCAL_IP,UDP_IP,UDP_PORT)

