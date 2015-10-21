import socket
import sys
import configparser
from xmlrpc.server import SimpleXMLRPCServer
from xmlrpc.server import SimpleXMLRPCRequestHandler
import errno
import math

config = configparser.ConfigParser()
config.read('host_daemon.conf')

HOST = ''	# Symbolic name, meaning all available interfaces
PORT = int(config['DEFAULT']['port'])
TOT_SIZE = int(config['DEFAULT']['total_size'])
BLOCK_SIZE = int(config['DEFAULT']['block_size'])
SHM_PATH = config['DEFAULT']['shm_path']

# Initialise th server data structures
num_blocks = math.floor(TOT_SIZE / BLOCK_SIZE)
free_blocks = list(range(num_blocks))
# A dictionary of ShmBlockInfos for each occupied block indexed by shmid
occupied_blocks = {}


# block_no
# wr_count
# wr_exc
# vmproc_info_list
class ShmBlockInfo:
    def __init__(self, block_no, shm_request_info):
        self.block_no = block_no
        self.wr_count = 0
        self.wr_exc = False;
        if (shm_request_info.rw_perms == 'w'):
            self.wr_exc = shm_request_info.wr_exc
            self.wr_count = 1
        self.vmproc_info_list = {(shm_request_info.vm_id, shm_request_info.pid) : shm_request_info}

class ShmBlockVMInfo:
    def __init__(self, vm_id, pid, rw_perms, wr_exc):
        self.vm_id = vm_id
        self.pid = pid
        self.rw_perms = rw_perms
        self.wr_exc = wr_exc

# TODO: Serialize these functions. I believe SimpleXMLRPCServer is already sequential by default

# returns err_code, start_address, size in bytes
def openShm(shmid, vm_id, pid, rw_perms, wr_exc):
    shm_request_info = ShmBlockVMInfo(vm_id, pid, rw_perms, wr_exc)
    # Check if already open
    if shmid in occupied_blocks:
        # if open, check flags and perms
        # if read only
        occupied_block = occupied_blocks[shmid]
        # check if same process already opened
        if (vm_id,pid) in occupied_block.vmproc_info_list:
            return (errno.EACCES, -1, -1)
        if (rw_perms == 'w'):
            if (occupied_block.wr_exc):
                return (errno.EACCES, -1, -1)
            else:
                occupied_block.wr_count = occupied_block.wr_count + 1
                occupied_block.wr_exc = wr_exc
        occupied_block.vmproc_info_list[(vm_id, pid)] = shm_request_info
        return (0, BLOCK_SIZE * occupied_block.block_no, BLOCK_SIZE)

    # If not open, open first time
    if (len(free_blocks) < 0):
        return (errno.ENOSPC, -1, -1);
    free_block = free_blocks[0]
    del free_blocks[0]
    occupied_blocks[shmid] = ShmBlockInfo(free_block, shm_request_info)
    addr = BLOCK_SIZE * free_block;
    return (0, addr, BLOCK_SIZE)


# returns an error code
def closeShm(shmid, vm_id, pid):
    if shmid in occupied_blocks:
        occupied_block = occupied_blocks[shmid]
        # check if requester opened the shmid
        if (vm_id, pid) in occupied_block.vmproc_info_list:
            blockVmInfo = occupied_block.vmproc_info_list[(vm_id, pid)]

            del occupied_block.vmproc_info_list[(vm_id, pid)]
            # If the requester is a writer
            if occupied_block.rw_perms == 'w':
                occupied_block.wr_count = occupied_block.wr_count - 1
                if occupied_block.wr_exc:
                    occupied_block.wr_exc = False

            # If ref_count == 0, delete the shared block
            if len(occupied_block.vmproc_info_list) == 0:
                free_blocks.append(occupied_block.block_no)
                del occupied_blocks[shmid]
            return 0

    # If not open, return error
    return errno.EPERM


# Restrict to a particular path.
class RequestHandler(SimpleXMLRPCRequestHandler):
    rpc_paths = ('/RPC2',)

# Create server
server = SimpleXMLRPCServer(("localhost", PORT),
                            requestHandler=RequestHandler)
server.register_introspection_functions()

# Register RPC functions
server.register_function(openShm, "openShm")
server.register_function(closeShm, "closeShm")

# Run the server's main loop
server.serve_forever()
