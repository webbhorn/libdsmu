package main;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;

public class PrintServer {
    private static ServerSocket servSock;
    private static final int PORT = 4444; // Port to listen on.
    
    // Print all messages received from clients to standard out.
    public static void main(String[] args) {
        try {
            servSock = new ServerSocket(PORT);
            while (true) {
                Socket clientSock = servSock.accept();
                PrintThread pt = new PrintThread(clientSock);
                new Thread(pt).start();
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
       
        
    }
    
}
