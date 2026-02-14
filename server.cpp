#include <iostream>
#include <string>

#include <thread>
#include <mutex>
#include <condition_variable>

#include <WinSock2.h>
#include <WinDef.h>
#include <WS2tcpip.h>
#include <tchar.h>

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

// boilerplate for Winsock on windows
bool initializeWinsock(){
    WSADATA wsa;
    int startup = WSAStartup(MAKEWORD(2,2), &wsa); // returns 0 on success

    if(startup != 0) return false;
    return true;
}

void cleanupSocketConnection(){
    WSACleanup();
}


int main(){

    if(!initializeWinsock()){
        std::cerr << "Winsock initialization failed" << "\n";
        return 1;
    }

    // initialize Socket, parameter- domain, type, protocol
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if(serverSocket == SOCKET_ERROR){
        std::cerr << "initialising socket failed" << "\n";
        cleanupSocketConnection();
        return 1;
    }

    // create socket address structure
    const int PORT = 8080;
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);

    // convert IP address to binary format using API for sin_addr
    if(InetPton(AF_INET, _T("0.0.0.0"), &serverAddress.sin_addr) != 1){
        std::cerr << "setting socket address structure failed" << "\n";
        cleanupSocketConnection();
        return 1;
    }

    // bind the IP & port with socket
    int socketBind = bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof serverAddress);

    if(socketBind == SOCKET_ERROR){
        std::cerr << "Socket binding failed" << "\n";
        cleanupSocketConnection();
        return 1;
    }

    std::cout << "Socket connection successfully established" << "\n";

    // listen on the connection
    int listenSocket = listen(serverSocket, SOMAXCONN);

    if(listenSocket == SOCKET_ERROR){
        std::cerr << "Socket listen failed" << "\n";
        cleanupSocketConnection();
        return 1;
    }

    std::cout << "Socket connection has started listening on port:" << PORT << "\n";

    // accept request from client
    SOCKET clientSocket = accept(serverSocket, NULL, NULL);

    if(clientSocket == SOCKET_ERROR){
        std::cerr << "client socket invalid" << "\n";
        cleanupSocketConnection();
        return 1;
    }

    // 4 steps completed- create, bind, listen, accept


    // receive the request from clientSocket after accepting request
    char buffer[4096];  
    int dataReceived = recv(clientSocket, buffer, sizeof buffer, 0);

    if(dataReceived == SOCKET_ERROR){
        std::cerr << "client data is invalid" << "\n";
        cleanupSocketConnection();
        return 1;
    }

    std::string clientMessage(buffer, dataReceived); 

    std::cout << "Message received from client: " << clientMessage << "\n";

    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();

}
