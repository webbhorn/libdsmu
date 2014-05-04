import socket
import os
import threading

PORT = 4444
MAXCONNREQUESTS = 5

class InvalidateServer:
	def __init__(self, port):
		self.port = port
		self.serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.clients = []
		self.lock = threading.Lock()

	def Listen(self):
		self.serverSocket.bind(('localhost', self.port))
		self.serverSocket.listen(MAXCONNREQUESTS)

		print "[InvalidateTest] Waiting for clients..."

		while True:
			(clientsocket, address) = self.serverSocket.accept()
			self.lock.acquire()
			self.clients.append(clientsocket)
			print "[InvalidateTest] Accepted client with address: " + str(address)
			self.lock.release()

	def SendInvalidate(self):
		while True:
			pageNum = raw_input("Enter a page number to invalidate: ")
			self.lock.acquire()
			message = "INVALIDATE " + pageNum # pageNum already a string
			for clientSock in self.clients:
				clientSock.send(message)
			for clientSock in self.clients:
				recvThread = threading.Thread(target = server.ReceiveInvalidate,
					args = (clientSock, ))
				recvThread.start()
			self.lock.release()

	def ReceiveInvalidate(self, socket):
		data = socket.recv(4096)
		print "[InvalidateTest] received from: " + str(socket.getpeername()) + " data: " + data


if __name__ == "__main__":
	try:
		server = InvalidateServer(PORT)

		print "Starting listen thread"
		listenThread = threading.Thread(target = server.Listen, args = ())
		listenThread.start()

		print "Starting invalidate thread"
		invalidateThread = threading.Thread(target = server.SendInvalidate, args = ())
		invalidateThread.start()

	except KeyboardInterrupt:
		server.serverSocket.close()
        for sock in server.clients:
            sock.close()
	    os._exit(0)
