import xmlrpclib

import argparse

parser = argparse.ArgumentParser(description='Process some integers.')

parser.add_argument('ip', metavar='ip', type=str, help='IP of the host daemon')
parser.add_argument('port', metavar='port', type=int, help='Port of the host daemon')

args = parser.parse_args()

s = xmlrpclib.ServerProxy(args.ip + ':' + str(args.port))
print s.pow(2,3)  # Returns 2**3 = 8
print s.add(2,3)  # Returns 5
print s.div(5,2)  # Returns 5//2 = 2