/* 
    creating client connection to send request to server

    steps-
    1. create socket
    2. connect to PORT and ip
    3. send the request / receive the request
    4. close the connection

*/

#include <iostream>

#include <WinSock2.h>
#include <WinDef.h>
#include <WS2tcpip.h>
#include <tchar.h>

bool initializeWinsock(){
    WSADATA wsa;
    int startup = WSAStartup(MAKEWORD(2,2), &wsa);

    if(startup != 0) return false; //
    return true;
}

int main(){
    initializeWinsock();

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if(clientSocket == SOCKET_ERROR){
        std::cerr << "socket initialization failed\n";
        return 1;
    }

    sockaddr_in serverAddress;
    const int PORT = 8080;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);


    if(InetPton(AF_INET, _T("0.0.0.0"), &serverAddress.sin_addr) != 1){
        std::cerr << "setting socket address structure failed" << "\n";
        return 1;
    }

    int serverConnect = connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof serverAddress);

    if(serverConnect == SOCKET_ERROR){
        std::cerr << "Server connection failed\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "successfully connected to server\n";

    // send / receive the message
    std::string message = "Hello! from client";

    int dataSent = send(clientSocket, message.c_str(), message.size(), 0); 

    if(dataSent == SOCKET_ERROR){
        std::cerr << "data not sent successfully\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    } 
}