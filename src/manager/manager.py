import os
import socket
from threading import Lock
from threading import Thread
import time

PORT = 4444
NUMPAGES = 100
MAXCONNREQUESTS = 5

# PERMISSION TYPES
NONE = 0
READ = 1
WRITE = 2

class PageTableEntry:
  def __init__(self):
    self.lock = Lock()
    self.readers = []
    self.owner = None # last processor to have write access
    self.current_permission = NONE
    self.invalidate_confirmations = {}
    self.send_page_confirmation = False

class ManagerServer:
  def __init__(self, port, numPages):
    self.global_lock = Lock()
    self.port = port
    self.clients = {} # client ids => ip addresses
    self.page_table_entries = [PageTableEntry() for i in range(numPages)]
    self.serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.id = 0

  def Listen(self):
    self.serverSocket.bind(('localhost', self.port))
    self.serverSocket.listen(MAXCONNREQUESTS)
    # MAXCONNREQUESTS represents the number of outstanding clients waiting to
    # interact with the server before clients are turned away. It is 5 because
    # that is the normal default, but setting it to the # of clients is ok too.
    print "[Manager Server] Waiting..."

    while True:
      # A client exists as long as the ManagerServer has a TCP connection to it.
      # Therefore, when a client connects here, we make a new thread to handle
      # its requests.
      (clientSocket, address) = self.serverSocket.accept()
      print "[Server] Accepted client with address: " + str(address)

      # Here, we just make a new thread to handle this client and run it, and
      # get back to waiting for new clients.
      clientThread = Thread(target = self.HandleClient, args = (clientSocket,
                            self.id, ))
      self.id += 1 # id used to uniquely identify print statements
      clientThread.start()


  # pass requests along to correct method in a new thread
  # serve Add/Remove clients requests
  # serve RequestPage requests
  # receive invalidate/sendpage confirmations
  # Call appropriate method in a new thread
  def HandleClient(self, clientSocket, idParam):
    # running in a new thread, handle the client
    idNum = idParam
    clientSocket.send("Welcome to the manager!!!!!")
    time.sleep(3)
    clientSocket.send("INVALIDATE AT 12340000 LEN 4096");
    while True:
      data = clientSocket.recv(7000) # Receive simple data, 4096 bytes here
      if not data:
        continue
      print "[HandleClient id " + str(idNum) + "] " + data # Print it out

  def AddClient(self, client, address):
    self.global_lock.Acquire()
    self.clients[client] = address
    self.global_lock.Release()

  def RemoveClient(self, client):
    self.global_lock.Acquire()
    del self.clients[client]
    self.global_lock.Release()

  def Invalidate(self, pagenumber):
    # Helper method
    # Tell clients using the page to invalidate, wait for confirmation
    page_table_entry = self.page_table_entries[pagenumber]
    page_table_entry.invalidate_confirmations = {}

    # TODO

    # Send invalidations to everyone (all readers + owner)

    # Use dictionary above to check which ones didn't confirm, request again

    # When all have confirmed, return

    pass

  def InvalidateConfirmation(self, client, pagenumber):
    # Alert invalidate thread
    page_table_entry = self.page_table_entries[pagenumber]
    page_table_entry.invalidate_confirmations[client] = True

  def SendPage(self, receiver, pagenumber):
    # Helper Method
    # Get page from owner
    # And send it to the requesting client
    # Wait for confirmation
    page_table_entry = self.page_table_entries[pagenumber]
    page_table_entry.send_page_confirmation = False

    # TODO

    pass

  def SPConfirmation(self, pagenumber):
    # Alert send page thread
    page_table_entry = self.page_table_entries[pagenumber]
    page_table_entry.send_page_confirmation = True

  def SendConfirmation(self, client, permission):
    # Sends read/write permission to client letting them know they may proceed

    # TODO

    pass

  def RequestPage(self, client, pagenumber, permission):
    # Invalidate page with other clients (if necessary)
    # Make sure client has latests page, ask other client to send page if necessary
    page_table_entry = self.page_table_entries[pagenumber]
    page_table_entry.lock.Acquire()

    # Initial use of page FAULT HANDLER
    if page_table_entry.current_permission == NONE:
      page_table_entry.current_permission = permission
      page_table_entry.owner = client
      self.SendConfirmation(client, permission)
      
      if permission == READ:
        page_table_entry.readers = [client]

      page_table_entry.lock.Relase()
      return

    # READ FAULT HANDLER
    if permission == READ:
      if page_table_entry.current_permission == WRITE:
        self.Invalidate(pagenumber)
        self.SendPage(client, permission)
        page_table_entry.readers = [client]
      else:
        page_table_entry.readers += client

    # WRITE FAULT HANDLER
    if permission == WRITE:
      self.Invalidate(pagenumber)
      self.SendPage(client, permission)
      page_table_entry.readers = []
      page_table_entry.owner = client

    self.SendConfirmation(client, permission)
    page_table_entry.current_permission = permission
    page_table_entry.lock.Release()
    return


if __name__ == "__main__":
  try:
    manager = ManagerServer(PORT, NUMPAGES)
    manager.Listen()
  except KeyboardInterrupt:
    os._exit(0)

