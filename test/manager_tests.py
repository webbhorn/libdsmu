import socket
from threading import Thread
import time
import os

WRITE = "WRITE"
READ = "READ"

class Client:
  def __init__(self, name):
    self.name = name
    self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.socket.connect(('localhost', 4444))
    Thread(target = self.listen).start()
    

  def listen(self):
    while True:
      data = self.socket.recv(7000)
      print "[%s] %s" % (self.name, data)
      args = data.split(" ")
      if args[0] == "INVALIDATE":
        self.socket.sendall("INVALIDATE CONFIRMATION " + args[1])

  def request_page(self, permission, pagenum):
    self.socket.sendall("REQUESTPAGE " + permission + " " + str(pagenum))

if __name__ == '__main__':
  c1 = Client("A")
  c2 = Client("B")

  c1.request_page(READ, 1)
  c2.request_page(READ, 1)

  time.sleep(2)

  c2.request_page(WRITE, 1)

  time.sleep(2)
  os._exit(0)
