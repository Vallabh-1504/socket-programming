#include <iostream>
#include <string>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstring>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


/*
    making a socket networking code which will open a TCP connection using sockets, listens to a port and an IP, accepts the connection and responds to the connection.

    A thread will spawn, listen to the port, and serve the request

    will be a blocking I/O, thread will sit and wait for the request

    steps-
    1. initialize the Winsock library
    2. create the socket
    3. get IP (localhost) and Port
    4. listen for request on Port
    5. accept the request
    6. send the response

    flow- create, bind, listen, accept, recieve, send

*/


// one thread to serve one client, will continuously listen for this client and will be waiting (blocking I/O)

std::mutex cout_mutex;

void serveClient(int clientSocket){
    std::cout << "client connected Successfully\n";

    // receive the request from clientSocket after accepting request
    // receive the request indefinitely

    char buffer[4096];  
    
    while(true){
        // thread will wait here, Blocking design
        int dataReceived = recv(clientSocket, buffer, sizeof buffer, 0);

        if(dataReceived == -1){
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cerr << "recv failed: " << strerror(errno) << "\n";
            break;
        }

        if (dataReceived == 0) {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Client disconnected." << "\n";
            break;
        }

        std::string clientMessage(buffer, dataReceived); 
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Message received from client: " << clientMessage << "\n";
        }

        // Send a response back to the client
        std::string response = "Message received.";
        send(clientSocket, response.c_str(), response.size(), 0);
    }

    close(clientSocket);

}


int main(){
    // initialize Socket, parameter- domain, type, protocol
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if(serverSocket == -1){
        std::cerr << "initialising socket failed" << "\n";
        return 1;
    }

    // create socket address structure
    const int PORT = 8080;
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);

    // convert IP address to binary format using API for sin_addr
    if(inet_pton(AF_INET, ("0.0.0.0"), &serverAddress.sin_addr) != 1){
        std::cerr << "setting socket address structure failed" << "\n";
        return 1;
    }

    // Allow reusing the address to avoid "bind failed" errors on restart
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // bind the IP & port with socket
    int socketBind = bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof serverAddress);

    if(socketBind == -1){
        std::cerr << "Socket binding failed" << "\n";
        return 1;
    }

    std::cout << "Socket connection successfully established" << "\n";

    // listen on the connection
    int listenSocket = listen(serverSocket, SOMAXCONN);

    if(listenSocket == -1){
        std::cerr << "Socket listen failed" << "\n";
        return 1;
    }

    std::cout << "Socket connection has started listening on port:" << PORT << "\n";

    // accept request from client, indefinitely
    while(true){
        int clientSocket = accept(serverSocket, NULL, NULL);

        if(clientSocket == -1){
            std::cerr << "client socket invalid" << "\n";
            return 1;
        }

        // 4 steps completed- create, bind, listen, accept

        // spawn a worker thread to serve the client so main thread can receive another request
        std::thread(serveClient, clientSocket).detach();
    }


    close(serverSocket);
}
