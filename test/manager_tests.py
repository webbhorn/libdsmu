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
    self.page = "FAKEPAGEDATA"
    Thread(target = self.listen).start()
    

  def listen(self):
    while True:
      data = self.socket.recv(7000)
      args = data.split(" ")
      if len(args) > 1 and args[1] == "INVALIDATE":
        print "[%s] %s" % (self.name, data)
        if len(args) > 3:
          string = "INVALIDATE CONFIRMATION " + args[2] + " " + self.page
          self.socket.sendall(str(len(string))+ " " + string)
        else:
          string = "INVALIDATE CONFIRMATION " + args[2] 
          self.socket.sendall(str(len(string))+ " " + string)


  def request_page(self, permission, pagenum):
    string = "REQUESTPAGE " + permission + " " + str(pagenum)
    strr = str(len(string))+ " " + string
    print strr
    self.socket.sendall(strr)

if __name__ == '__main__':
  c1 = Client("A")
  c2 = Client("B")
  #c3 = Client("C")

  print "reading page" 
  c1.request_page(READ, 1)
  c2.request_page(READ, 1)
  #c3.request_page(READ, 1)

  time.sleep(.1)

  c2.request_page(WRITE, 1)
  c2.page = "ADSDFASFDASF"

  time.sleep(.1)

  c1.request_page(WRITE, 1)

  time.sleep(1)
  os._exit(0)
