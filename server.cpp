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

// new headers for non-blocking I/O and epoll
#include <fcntl.h>
#include <sys/epoll.h>


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

// BOILER PLATE for non-blocking
bool set_nonblocking(int sockfd){
    // 1.  Get current file status flags
    int flags = fcntl(sockfd, F_GETFL, 0);
    if(flags == -1){
        return false;
    }
    
    // 2. Add the non-blocking flag using bitwise OR
    flags = flags | O_NONBLOCK;

    if(fcntl(sockfd, F_SETFL, flags) == -1){
        return false;
    }

    return true;
}

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

    // --- Non-blocking code changes ---
    
    // 1. Create epoll instance in kernal
    int epoll_fd = epoll_create1(0);
    if(epoll_fd == -1){
        std::cerr << "Failed to create epoll file descriptor\n";
        close(serverSocket);
        return 1;
    }

    std::cout << "Epoll instance created successfully!\n";

    // 2. Make Server socket non-blocking
    if(!set_nonblocking(serverSocket)){
        std::cerr << "Failed to set server socket to non-blocking\n";
        close(serverSocket);
        return 1;
    }

    // 3. Register server socket with epoll
    struct epoll_event event;
    event.events = EPOLLIN; // We want to know when there is INcoming data (a new connection)
    event.data.fd = serverSocket; // This is the socket we are registering

    // epoll_ctl adds, modifies, or deletes sockets from the epoll instance
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverSocket, &event) == -1){
        std::cerr << "Failed to add server socket to epoll\n";
        close(serverSocket);
        return 1;
    }

    // 4. Reactor event loop
    const int MAX_EVENTS = 10; // Maximum number of events to process at once
    struct epoll_event events[MAX_EVENTS]; // Array to hold the events that just happened
    
    std::cout << "Reactor loop starting. Waiting for events...\n";


    // accept request from client, indefinitely
    while(true){
        // This will block the thread until AT LEAST one event happens.
        // '-1' means "wait indefinitely" (no timeout).
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        if(num_events == -1){
            std::cerr << "epoll_wait error\n";
            break;
        }
        
        // Using epoll_wait instead of accept
        // int clientSocket = accept(serverSocket, NULL, NULL);
        // if(clientSocket == -1){
        //     std::cerr << "client socket invalid" << "\n";
        //     return 1;
        // }

        // 4 steps completed- create, bind, listen, accept (epoll_wait)

        // old blocking code
        // // spawn a worker thread to serve the client so main thread can receive another request
        // std::thread(serveClient, clientSocket).detach();

        // loop through all socket that kernel says are ready
        for(int i = 0;i < num_events; i++){
            // CASE 1: socket that triggered event is our main listening socket
            // This means a new client is trying to connect
            if(events[i].data.fd == serverSocket){
                std::cout << "[EVENT] New connection attempt detected on server socket!\n";

                // Handling this later, accept for now so is does not spam
                int client_socket = accept(serverSocket, NULL, NULL);
                if(client_socket == -1){
                    std::cerr << "client socket Accept failed\n";
                    continue;
                } 

                // 1. make the new client non-blocking
                int flags = fcntl(client_socket, F_GETFL, 0);
                fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

                // 2. register the client with epoll
                struct epoll_event client_event;
                client_event.events = EPOLLIN | EPOLLET;
                client_event.data.fd = client_socket;

                if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &client_event) == -1){
                    std::cerr << "Failed to add client to epoll\n";
                    close(client_socket);
                }
                else {
                    std::cout << "[EVENT] New client connected and registered with epoll! FD: " << client_socket << "\n";
                }
            }

            // CASE 2: socket that triggered event is a connected client socket, this means a client sent us data
            else{
                std::cout << "[EVENT] data arrived on a client socket\n";

                int clientFd = events[i].data.fd;
                char buffer[4096];

                // Read the data (non-blocking) until EAGAIN is returned
                while(true){
                    int bytesReceived = recv(clientFd, buffer, sizeof(buffer), 0);

                    if(bytesReceived > 0){
                        std::string clientMessage(buffer, bytesReceived); 
                        std::cout << "[EVENT] Data from FD " << clientFd << ": " << clientMessage << "\n";
                        
                        // Echo response back
                        std::string response = "Message received by Reactor.";
                        send(clientFd, response.c_str(), response.size(), 0);
                    } 
                    else if(bytesReceived == 0){
                        std::cout << "[EVENT] Client on FD " << clientFd << " disconnected.\n";
                        close(clientFd); // Automatically removes from epoll
                    } 
                    else {
                        // ET check, when buffer is drained, we get -1 and errno EAGAIn
                        if(errno == EAGAIN || errno == EWOULDBLOCK){
                            // Buffer is empty. Break the read loop and go back to epoll_wait
                            break;
                        }
                        else{
                            // a real error occured
                            std::cerr << "[EVENT] Error on FD " << clientFd << "\n";
                            close(clientFd);
                        }
                    }
                }
            }
        }
    }

    close(serverSocket);
    return 0;
}
