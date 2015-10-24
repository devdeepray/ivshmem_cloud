import select
import socket
import sys
from queue import Queue
import struct
from collections import namedtuple
import math
import errno



HOSTNAME = 'localhost'
PORT = 8888

REQUEST_PROTO_STRUCT = 'iiii'
REQUEST_PROTO_SIZE = struct.calcsize(REQUEST_PROTO_STRUCT)
Request = namedtuple('Request', 'req_type id uid wr_perm')

ALLOC_PROTO_STRUCT = 'iii'
ALLOC_PROTO_SIZE = struct.calcsize(ALLOC_PROTO_STRUCT)
Alloc = namedtuple('Alloc', 'size offset errno')

RET_PROTO_STRUCT = 'i'
RET_PROTO_SIZE = struct.calcsize(RET_PROTO_STRUCT)
Ret = namedtuple('Ret', 'errno')

TOT_SIZE = 512
BLOCK_SIZE = 1
SHM_PATH = 'dummy'

# Initialise th server data structures
NUM_BLOCKS = math.floor(TOT_SIZE / BLOCK_SIZE)

free_blocks = list(range(NUM_BLOCKS))
# A dictionary of ShmBlockInfos for each occupied block indexed by shmid
occupied_blocks = {}

# CV stuff
# CVID : opened CV map
opened_dict = {}
# CVID : uid
acquired_dict = {}
# CVID : [uid]
waiting_dict = {}
# CVID : [uid]
acq_pending_dict = {}
# UID : socket
uid_socket_dict = {}

# block_no
# wr_count
# wr_exc
# vmproc_info_list
class ShmBlockInfo:
    def __init__(self, block_no, shm_request_info):
        self.block_no = block_no
        self.wr_count = 0
        self.wr_exc = False;
        if (shm_request_info.rw_perms <= 2):
            self.wr_exc = shm_request_info.rw_perms == 1
            self.wr_count = 1
        self.vmproc_info_list = {shm_request_info.uid : shm_request_info}

class ShmBlockVMInfo:
    def __init__(self, uid, rw_perms):
        self.uid = uid
        self.rw_perms = rw_perms

# TODO: Serialize these functions. I believe SimpleXMLRPCServer is already sequential by default

def process_alloc(shmid, uid, rw_perms):
    shm_request_info = ShmBlockVMInfo(uid, rw_perms)
    # Check if already open
    if shmid in occupied_blocks:
        # if open, check flags and perms
        # if read only
        occupied_block = occupied_blocks[shmid]
        # check if same process already opened
        if uid in occupied_block.vmproc_info_list:
            return struct.pack(ALLOC_PROTO_STRUCT, errno.EACCES, -1, -1)
        if (rw_perms <= 2):
            if (occupied_block.wr_exc):
                return struct.pack(ALLOC_PROTO_STRUCT, errno.EACCES, -1, -1)
            else:
                occupied_block.wr_count = occupied_block.wr_count + 1
                occupied_block.wr_exc = rw_perms == 1
        occupied_block.vmproc_info_list[uid] = shm_request_info
        return struct.pack(ALLOC_PROTO_STRUCT, 0, BLOCK_SIZE * occupied_block.block_no, BLOCK_SIZE)

    # If not open, open first time
    if (len(free_blocks) < 0):
        return struct.pack(ALLOC_PROTO_STRUCT, errno.ENOSPC, -1, -1);
    free_block = free_blocks[0]
    del free_blocks[0]
    occupied_blocks[shmid] = ShmBlockInfo(free_block, shm_request_info)
    addr = BLOCK_SIZE * free_block;
    return struct.pack(ALLOC_PROTO_STRUCT, 0, addr, BLOCK_SIZE)

def process_dealloc(shmid, uid):
    if shmid in occupied_blocks:
        occupied_block = occupied_blocks[shmid]
        # check if requester opened the shmid
        if uid in occupied_block.vmproc_info_list:
            blockVmInfo = occupied_block.vmproc_info_list[uid]

            del occupied_block.vmproc_info_list[uid]
            # If the requester is a writer
            if blockVmInfo.rw_perms <= 2:
                occupied_block.wr_count = occupied_block.wr_count - 1
                if occupied_block.wr_exc:
                    occupied_block.wr_exc = False

            # If ref_count == 0, delete the shared block
            if len(occupied_block.vmproc_info_list) == 0:
                free_blocks.append(occupied_block.block_no)
                del occupied_blocks[shmid]
            return struct.pack(RET_PROTO_STRUCT, 0)

    # If not open, return error
    return struct.pack(RET_PROTO_STRUCT, errno.EPERM)

def process_create_cv(id, uid):
    # Check if Cv already created
    if id not in opened_dict:
        waiting_dict[id] = []
        acq_pending_dict[id] = []
        opened_dict[id] = [uid]
    elif uid in opened_dict[id]:
        return struct.pack(RET_PROTO_STRUCT, errno.EPERM)
    else:
        opened_dict[id].append(uid)
    return struct.pack(RET_PROTO_STRUCT, 0);

def process_acq_cv(id, uid):
    if id not in opened_dict:
        return struct.pack(RET_PROTO_STRUCT, errno.EPERM)
    elif id in acquired_dict:
        if acquired_dict[id] == uid:
            return struct.pack(RET_PROTO_STRUCT, errno.EPERM)
        else:
            acq_pending_dict[id].append(uid)
            return b''
    else:
        acquired_dict[id] = uid
        return struct.pack(RET_PROTO_STRUCT, 0)

def process_rel_cv(id, uid):
    if id not in acquired_dict or uid != acquired_dict[id]:
        return struct.pack(RET_PROTO_STRUCT, errno.EPERM)

    del acquired_dict[id]

    if len(acq_pending_dict[id]) > 0:
        tmp = acq_pending_dict[id].pop(0)
        uid_socket_dict[tmp].send(process_acq_cv(id, tmp))

    return struct.pack(RET_PROTO_STRUCT, 0);

def process_wait_cv(id, uid):
    if id not in acquired_dict or uid != acquired_dict[id]:
        return struct.pack(RET_PROTO_STRUCT, errno.EPERM)

    del acquired_dict[id]

    waiting_dict[id].append(uid)

    if len(acq_pending_dict[id]) > 0:
        tmp = acq_pending_dict[id].pop(0)
        uid_socket_dict[tmp].send(process_acq_cv(id, tmp))

    return struct.pack(RET_PROTO_STRUCT, 0)

def process_notify_cv(id, uid):
    if id not in acquired_dict or uid != acquired_dict[id]:
        return struct.pack(RET_PROTO_STRUCT, errno.EPERM)

    if len(waiting_dict[id]) > 0:
        tmp = waiting_dict[id].pop(0)
        acq_pending_dict[id].append(tmp)

    return struct.pack(RET_PROTO_STRUCT, 0)

def process_del_cv(id, uid):
    if id in opened_cv and uid in opened_cv[id]:
        if id in acquired_dict and acquired_dict[id]==uid:
            return struct.pack(RET_PROTO_STRUCT, errno.EPERM)
        else:
            opened_cv[id].remove(uid)
            if len(opened_cv[id]) == 0:
                del opened_cv[id]
            return struct.pack(RET_PROTO_STRUCT, 0)
    return struct.pack(RET_PROTO_STRUCT, errno.EPERM)

def process_request(req, sock):
    request = Request._make(struct.unpack(REQUEST_PROTO_STRUCT, req))
    uid_socket_dict[request.uid] = sock
    if (request.req_type == 1):
        return process_alloc(request.id, request.uid, request.wr_perm)
    elif (request.req_type == 2):
        return process_dealloc(request.id, request.uid)
    elif (request.req_type == 3):
       return process_create_cv(request.id, request.uid)
    elif (request.req_type == 4):
       return process_acq_cv(request.id, request.uid)
    elif (request.req_type == 5):
       return process_rel_cv(request.id, request.uid)
    elif (request.req_type == 6):
        return process_wait_cv(request.id, request.uid)
    elif (request.req_type == 7):
        return process_notify_cv(request.id, request.uid)
    else:
        return process_del_cv(request.id, request.uid)

# Non blocking server socket
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setblocking(0)

# Bind to port
server_address = (HOSTNAME, PORT)
print('Starting server on ' + HOSTNAME + ':' + str(PORT))
server.bind(server_address)

# Start listening for connections
server.listen(10)

# Initialize lists for loop
inputs = [ server ]
outputs = []
message_queues = {}

# Loop for non blocking socket using UNIX select
while inputs:
    readable, writable, exceptional = select.select(inputs, outputs, inputs)
    #print('rd cnt = ' + str(len(readable)))
    #print('wr cnt = ' + str(len(writable)))
    #print('ex cnt = ' + str(len(exceptional)))
    for s in readable:
        if s is server:
            # Need to accept, because someone is trying to connect
            conn, addr = s.accept()
            #print('Connected from ' + str(addr))
            conn.setblocking(0)
            inputs.append(conn)
            message_queues[conn] = b''
        else:
            # Normal socket. Need to recv data
            data = s.recv(1024)
            if data:
                message_queues[s] = message_queues[s] + data
                if s not in outputs:
                    outputs.append(s)
            else:
                # empty result, socket closed. Invalidate pending
                #print('closing ' + str(s.getpeername()))
                if s in outputs:
                    outputs.remove(s)
                inputs.remove(s)
                del message_queues[s]
                if s in writable:
                    writable.remove(s)
                s.close()

    for s in writable:
        mlen = len(message_queues[s])
        if mlen == 0:
            outputs.remove(s)
        elif mlen >= REQUEST_PROTO_SIZE:
            req = message_queues[s][:REQUEST_PROTO_SIZE]
            message_queues[s] = message_queues[s][REQUEST_PROTO_SIZE:]
            s.send(process_request(req, s))
    for s in exceptional:
        print( 'handling exceptional condition for'+ s.getpeername())
        # Stop listening for input on the connection
        inputs.remove(s)
        if s in outputs:
            outputs.remove(s)
        s.close()

        # Remove message queue
        del message_queues[s]
