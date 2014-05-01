import Lock from thread

# PERMISSION TYPES
NONE = 0
READ = 1
WRITE = 2

class PageTableEntry:
  def __init__(self):
    self.lock = Lock()
    self.readers = []
    self.owner # last processor to have write access
    self.current_permission = NONE

class ManagerServer:
  def __init__(self, port, numPages):
    self.port = port
    self.clients = {} # client ids => ip addresses
    self.page_table_entries = [PageTableEntry() for i in range(numPages)]

  def Listen(self):
    # start listening on port
    print "[Manager Server] Waiting..."

    # pass requests along to correct method in a new thread
    # serve Add/Remove clients requests
    # serve RequestPage requests
    while True:
      pass

  def AddClient(self, client, address):
    self.clients[client] = address

  def RemoveClient(self, client):
    del self.clients[client]

  def Invalidate(self, pagenumber):
    # Helper method
    # Tell clients using the page to invalidate, wait for confirmation
    pass

  def SendPage(self, receiver, pagenumber):
    # Helper Method
    # Get page from owner
    # And send it to the requesting client
    # Wait for confirmation
    pass

  def SendConfirmation(self, client, permission):
    # Sends read/write permission to client letting them know they may proceed
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
  manager = ManagerServer(4444)
  manager.Listen()
