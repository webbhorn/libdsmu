import socket
import os
import threading
from threading import Thread

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
            clientThread = Thread(target = self.HandleClient, args = (clientsocket, ))
            clientThread.start()

    def HandleClient(self, clientSocket):
        while True:
            data = clientSocket.recv(7000)
            if not data:
                continue
            print data

    def SendInvalidate(self):
        while True:
            pageNum = raw_input("> ")
            if pageNum == "exit":
                break # Allow exit input

            self.lock.acquire()
            message = pageNum
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

    def Cleanup(self):
        self.serverSocket.close()
        for sock in server.clients:
            sock.close()

        


if __name__ == "__main__":
    server = InvalidateServer(PORT)
    try:
        print "Starting listen thread"
        listenThread = threading.Thread(target = server.Listen, args = ())
        listenThread.start()

        print "Starting invalidate thread"
        invalidateThread = threading.Thread(target = server.SendInvalidate, args = ())
        invalidateThread.start()
        
        invalidateThread.join() # Quit when the invalidate thread returns (on "exit" input)

        print "Calling cleanup"        
        server.Cleanup()
        os._exit(0)
    except KeyboardInterrupt:
        server.Cleanup()
        os._exit(0)
