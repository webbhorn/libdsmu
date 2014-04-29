package main;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.Socket;

public class PrintThread implements Runnable {
    private final Socket connection;
    
    public PrintThread(Socket conn) {
        connection = conn;
    }
    
    @Override
    public void run() {
        try ( // Try with resources: closes input for you!
            BufferedReader input = new BufferedReader(new InputStreamReader(connection.getInputStream()));
        ) { 
            for (String line = input.readLine(); line != null; line = input.readLine()) {
                System.out.println("Message received from client: " + line);
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        
        try {
            connection.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        
    }
    
}
