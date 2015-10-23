import rpyc
from time import time
from threading import Thread

NUM_THREADS = 1
NUM_REQ_PER_THREAD = 10000


def reqGen(tid):
	s = rpyc.connect('127.0.0.1', 8888, config={'allow_public_attrs': True})
	start = time()
	for i in range(0, NUM_REQ_PER_THREAD):
		s.root.openShm(i % 512, tid, tid, 'w', False)
		s.root.closeShm((i + 256) % 512, tid, tid)
	end = time()
	print ('rate = ' + str(NUM_REQ_PER_THREAD / (end - start)));


for i in range(0, NUM_THREADS):
	t = Thread(target=reqGen, args=(i, ))
	t.start()
