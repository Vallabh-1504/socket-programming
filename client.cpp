/* 
    creating client connection to send request to server

    steps-
    1. create socket
    2. connect to PORT and ip
    3. send the request / receive the request
    4. close the connection

*/

#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>

int main(){
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if(clientSocket == -1){
        std::cerr << "socket initialization failed\n";
        return 1;
    }

    sockaddr_in serverAddress;
    const int PORT = 8080;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);


    if(inet_pton(AF_INET, ("127.0.0.1"), &serverAddress.sin_addr) != 1){
        std::cerr << "setting socket address structure failed" << "\n";
        return 1;
    }

    int serverConnect = connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof serverAddress);

    if(serverConnect == -1){
        std::cerr << "Server connection failed\n";
        close(clientSocket);
        return 1;
    }

    std::cout << "successfully connected to server\n";

    // send / receive the message
    std::string message = "Hello! from client";

    int dataSent = send(clientSocket, message.c_str(), message.size(), 0); 

    if(dataSent == -1){
        std::cerr << "data not sent successfully\n";
        close(clientSocket);
        return 1;
    }

    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        std::cout << "Server response: " << buffer << std::endl;
    }

    message = "another Message";
    std::cout << "Sending second message..." << std::endl;
    send(clientSocket, message.c_str(), message.length(), 0);

    bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        std::cout << "Server response: " << buffer << std::endl;
    }

    close(clientSocket);
}