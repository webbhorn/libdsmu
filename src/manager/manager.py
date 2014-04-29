class ManagerServer:
  def __init__(self, port):
    self.port = port
    # dictionary of client ids => ip addresses
    self.clients = {}

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

  def Invalidate(self, client, pagenumber):
    # Helper method
    # Tell client to invalidate pagenumber, wait for confirmation
    pass

  def SendPage(self, sender, receiver, pagenumber):
    # Helper Method
    # Ask client to send page to other client
    # Wait for confirmation
    pass

  def RequestPage(self, client, pagenumber, permission):
    # Invalidate page with other clients (if necessary)
    # Make sure client has latests page, ask other client to send page if necessary
    pass


if __name__ == "__main__":
  manager = ManagerServer(4444)
  manager.Listen()
