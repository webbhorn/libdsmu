import os
import socket
import time
from threading import Thread

PORT = 4445
MAXCONNREQUESTS = 5

# The following is a simple python server that models the kind of communication
# the manager will be having with client machines.
# This example doesn't handle closing sockets or data structures, etc.
# It's meant to just illustrate that we have one thread accepting new clients,
# and then many client threads that THEMSELVES receive data from clients and
# handle requests.
class TestServer:
    def __init__(self, port):
        self.port = port
        # Set up an internet, TCP socket
        self.serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.id = 0

    def Listen(self):
        self.serverSocket.bind(('localhost', self.port))
        self.serverSocket.listen(MAXCONNREQUESTS)
        # MAXCONNREQUESTS represents the number of outstanding clients waiting to interact with the server
        # before clients are turned away. It is 5 because that is the normal default, but setting it to the # of clients is ok too.
        print "[Server] Waiting..."

        while True:
            # A client exists as long as the ManagerServer has a TCP connection to it.
            # Therefore, when a client connects here, we make a new thread to handle its requests.
            (clientSocket, address) = self.serverSocket.accept()
            print "[Server] Accepted client with address: " + str(address)

            # Here, we just make a new thread to handle this client and run it, and get back to waiting for new clients.
            clientThread = Thread(target = self.HandleClient, args = (clientSocket, self.id, ))
            self.id += 1 # id used to uniquely identify print statements
            clientThread.start()

    def HandleClient(self, clientSocket, idParam):
        # running in a new thread, handle the client
        idNum = idParam
        time.sleep(3)
        clientSocket.send("INVALIDATE 74560 PAGEDATA")
        while True:
            data = clientSocket.recv(4096) # Receive simple data, 4096 bytes here
            print "[HandleClient id " + str(idNum) + "] " + data # Print it out

if __name__ == "__main__":
  try:
    manager = TestServer(PORT)
    manager.Listen()
  except KeyboardInterrupt:
    manager.serverSocket.close()
    os._exit(0)

