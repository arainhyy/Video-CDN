#!/usr/bin/env python

import sys, random, time, string

# overload socket.create_connection
import socket
real_create_conn = socket.create_connection
def set_src_addr(*args):
    address, timeout = args[0], args[1]
    source_address = ('1.0.0.1', 0)
    return real_create_conn(address, timeout, source_address)
socket.create_connection = set_src_addr

import requests
from threading import Thread

client_addr = ('1.0.0.1', 0)
server_addr = ('3.0.0.1', random.randrange(1025,3000))

def client_to_webserver():
    print 'starting web test'
    for i in xrange(10):
        sys.stdout.write('sending request...')
        sys.stdout.flush()
        start = time.time()
        r = requests.get('http://3.0.0.1:8080/vod/1000Seg1-Frag2')
        end = time.time()
        cl = int(r.headers['content-length'])

        print 'BW: %d Kbps' % (int((cl*8 / float(1000)) / (end-start)))
    

def client_test():
    print 'starting direct tc test'
    client = socket.socket()
    client.bind(client_addr)
    client.connect(server_addr)

    REQUEST_SIZE = 4096

    try:
        for i in xrange(10):

            start = time.time()
            client.sendall(str(REQUEST_SIZE))
            resp = client.recv(REQUEST_SIZE+10)
            #print 'got back %d bytes' % len(resp)
            if len(resp) == 0: break
            end = time.time()

            print 'BW: %d Kbps' % (int((REQUEST_SIZE*8 / float(1000)) / (end-start)))
    finally:
        client.close()

def gen_random_string(size):
    return ''.join(random.choice(string.ascii_letters + string.digits) for x in range(size))


def server_test():
    server = socket.socket()
    server.bind(server_addr)
    server.listen(5)

    data = ''

    try:
        while True:
            try:
                (client_socket, client_information) = server.accept()
            except:
                server.close()
                break

            # serve this client only until it disconnects
            while True:
                try:
                    msg = client_socket.recv(4096)
                    requested_size = int(msg)
                    #print 'Returning %i bytes' % requested_size

                    if len(data) is not requested_size:
                        data = gen_random_string(requested_size)
                    client_socket.sendall(data)
                except Exception, e:
                    client_socket.close()
                    break
            break
    finally:
        server.close()

def local_test():
    s_t = Thread(target=server_test)
    s_t.start()
    time.sleep(1)
    c_t = Thread(target=client_test)
    c_t.start()
    c_t.join()
    s_t.join()

def web_test():
    client_to_webserver()

local_test()
web_test()
