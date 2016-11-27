#!/usr/bin/python
import os, struct, socket, time, random, select
from dijkstra import getBestServer

def s(i):
    return struct.pack('>H', i)
def b(i):
    return struct.pack('>B', i)
def us(i):
    return struct.unpack('>H', i)[0]
def ub(i):
    return struct.unpack('>B', i)[0]

SERVERS = []
SERV_CURR = 0

# DEFAULTS
TRANS_ID = us(os.urandom(2))
FLAGS_QR = 0 #Query
FLAGS_Opcode = 0 #Std query
FLAGS_AA = 1 #is an authority
FLAGS_TC = 0 #not truncated
FLAGS_RD = 0 #not asking for recursion
FLAGS_RA = 0 #recursion not allowed (or is a request)
FLAGS_Zero = 0 # always zero
FLAGS_RCode = 0 # no errors
FLAGS = 0

NUM_QUESTIONS = 1
NUM_ANSWERS = 0
NUM_AUTHORITY = 0
NUM_ADDITIONAL = 0

QType = 1 # A record
QClass = 1 # IN

LOG_FILE = None

def serverSetup(servers_file, log):
    global LOG_FILE
    global SERVERS
    SERVERS = open(servers_file).read().split('\n')
    if SERVERS[-1] == '':
        SERVERS = SERVERS[:-1]
    LOG_FILE = open(log, 'w', 0)

def genFlags(query):
    global FLAGS_QR, NUM_ANSWERS, FLAGS
    if not query:
        FLAGS_QR = 1
        NUM_ANSWERS = 1

    FLAGS_B1 = FLAGS_QR<<7 | FLAGS_Opcode <<3 | FLAGS_AA <<2 | FLAGS_TC <<1 | FLAGS_RD
    FLAGS_B2 = FLAGS_RA<<7 | FLAGS_RCode
    FLAGS = FLAGS_B1<<8 | FLAGS_B2

def genMessage(query_str, query=1, ROUND_ROBIN=0, servers_file="", lsa_file="", addr="", logfile=None):
    global SERV_CURR

    if not query and len(SERVERS) == 0:
        serverSetup(servers_file, logfile)

    genFlags(query)

    message = s(TRANS_ID)+s(FLAGS)+s(NUM_QUESTIONS)+s(NUM_ANSWERS)+s(NUM_AUTHORITY)+ \
        s(NUM_ADDITIONAL)

    # Calc lengths for Query string
    lens = []
    i = 0
    while i in xrange(len(query_str)):
        k = 0
        for j in xrange(i,len(query_str)):
            if query_str[j] == '.':
                lens.append(k)
                break
            k += 1
        i = j+1
    total_read = sum(lens)+len(lens)
    lens.append(len(query_str)-total_read)

    # Insert lengths and names into message
    message += b(lens[0])
    l = 1
    for i,c in enumerate(query_str):
        if c == '.':
            message += b(lens[l])
            l += 1
        else:
            message += struct.pack('c',c)
    message += b(0)

    message += s(QType)
    message += s(QClass)

    # build RR's
    RR_ADDR = ""
    if not query:
        # # Insert lengths and names into message
        # message += b(lens[0])
        # l = 1
        # for i,c in enumerate(query_str):
        #     if c == '.':
        #         message += b(lens[l])
        #         l += 1
        #     else:
        #         message += struct.pack('c',c)
        # message += b(0)
        RR_NAME = 49164 #C0 0C
        message += s(RR_NAME)

        RR_QTYPE = 1 #A record
        RR_QCLASS = 1 #IN
        RR_TTL = 0 # no caching
        RR_DATALENGTH = 4
        if ROUND_ROBIN:
            RR_ADDR = SERVERS[SERV_CURR]
            SERV_CURR = (SERV_CURR + 1) % len(SERVERS)
        else:
            RR_ADDR = getBestServer(addr[0], SERVERS, lsa_file)
        
        message += s(RR_QTYPE)+s(RR_QCLASS) + \
            struct.pack('>I',RR_TTL) + s(RR_DATALENGTH)
        RR_ADDR = [int(float(a)) for a in RR_ADDR.split('.')]
        for a in RR_ADDR:
            message += b(a)
        t = int(time.time())
        out_addr = '.'.join([str(r) for r in RR_ADDR])
        log_s= ' '.join([str(t),addr[0],query_str,out_addr])
        print log_s
        LOG_FILE.write(log_s+'\n')
    return (message, RR_ADDR)

def parseMessage(data, query=0):
    global TRANS_ID
    R_TRANS_ID = us(data[0:2])
    if query:
        TRANS_ID = R_TRANS_ID

    R_FLAGS = us(data[2:4])
    R_FLAGS_B1 = R_FLAGS >> 8
    R_FLAGS_B2 = R_FLAGS & 255

    R_FLAGS_QR = (R_FLAGS_B1 & 128) >> 7
    R_FLAGS_Opcode = (R_FLAGS_B1 & (15<<3)) >> 3
    R_FLAGS_AA = (R_FLAGS_B1 & 4) >> 2
    R_FLAGS_TC = (R_FLAGS_B1 & 2) >> 1
    R_FLAGS_RD = (R_FLAGS_B1 & 1)

    R_FLAGS_RA = (R_FLAGS_B2 & 128) >> 7
    R_FLAGS_Zero = (R_FLAGS_B2 & (7<<4)) >> 4
    R_FLAGS_RCode = R_FLAGS_B2 & 15

    R_NUM_QUESTIONS = us(data[4:6])
    R_NUM_ANSWERS = us(data[6:8])
    R_NUM_AUTHORITY = us(data[8:10])
    R_NUM_ADDITIONAL = us(data[10:12])

    R_INPUT = []
    i = 13
    l = ub(data[12])
    while l != 0:
        R_INPUT += data[i]
        i += 1
        l -= 1
        if l == 0:
            l = ub(data[i])
            i += 1
            R_INPUT += '.'
    R_INPUT = ''.join(R_INPUT[:-1])

    R_QType = us(data[i:i+2]) # A record
    i += 2
    R_QClass = us(data[i:i+2]) # IN
    i += 2

    # print R_FLAGS_QR, R_FLAGS_Opcode, R_FLAGS_AA, R_FLAGS_TC, R_FLAGS_RD
    # print R_FLAGS_RA, R_FLAGS_Zero, R_FLAGS_RCode
    # print R_NUM_QUESTIONS, R_NUM_ANSWERS, R_NUM_AUTHORITY, R_NUM_ADDITIONAL


    RR_ADDR = ""

    RR_NAME = ""
    if R_NUM_ANSWERS:
        if us(data[i:i+2]) == 49164: # C0 0C
            RR_NAME = us(data[i:i+2]) # should be C0 0C
            i += 2
        else:
            l = ub(data[i])
            i += 1
            while l != 0:
                RR_NAME += data[i]
                i += 1
                l -= 1
                if l == 0:
                    l = ub(data[i])
                    i += 1
                    RR_NAME += '.'
            RR_NAME = ''.join(RR_NAME[:-1])
            
        RR_TYPE = us(data[i:i+2]) # A record
        i += 2
        RR_CLASS = us(data[i:i+2]) # IN
        i += 2
        RR_TTL = (us(data[i:i+2])<<8) + (us(data[i+2:i+4]))
        i += 4
        RR_DL = us(data[i:i+2])
        i += 2
        addr = [ub(data[i:i+1]),ub(data[i+1:i+2]),ub(data[i+2:i+3]),ub(data[i+3:i+4])]
        addr = [str(a) for a in addr]
        RR_ADDR = '.'.join(addr)
        # print RR_NAME, RR_TYPE, RR_CLASS, RR_TTL, RR_DL

    flags = {}

    if not query:
        flags = {'sent_trans_id' : TRANS_ID, 'recv_trans_id' : R_TRANS_ID, 'qr' : R_FLAGS_QR, 
                 'opcode' : R_FLAGS_Opcode, 'aa' : R_FLAGS_AA, 'tc' : R_FLAGS_TC, 'rd' : R_FLAGS_RD,
                 'ra' : R_FLAGS_RA, 'z' : R_FLAGS_Zero, 'rcode' : R_FLAGS_RCode, 
                 'num_questions' : R_NUM_QUESTIONS, 'num_answers' : R_NUM_ANSWERS, 
                 'num_authority' : R_NUM_AUTHORITY, 'num_additional' : R_NUM_ADDITIONAL, 
                 'qtype' :R_QType, 'qclass' : R_QClass, 'rr_name' : RR_NAME, 'rr_qtype' : RR_TYPE, 
                 'rr_qclass' : RR_CLASS, 'rr_ttl' : RR_TTL, 'rr_dl' : RR_DL}

    return (R_INPUT, RR_ADDR, flags)

def sendDNSQuery(query, local_ip, server_ip, server_port):
    message = genMessage(query, 1)[0]
    dns_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    dns_sock.bind((local_ip,0))
    dns_sock.sendto(message, (server_ip, server_port))
    dns_sock.setblocking(0)
    data = []
    for i in xrange(2):
        ready = select.select([dns_sock], [], [], 1)
        if ready[0]:
            data, _ = dns_sock.recvfrom(1024)
            break
    if not data:
        raise Exception("DNS server failed to respond")
    return parseMessage(data, 0)
