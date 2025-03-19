#ifndef SOCKET_WRAPPER_H
#define SOCKET_WRAPPER_H

// Platform detection
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET socket_t;
#define SOCKET_ERROR_VALUE INVALID_SOCKET
#define SOCKET_CLOSE(s) closesocket(s)
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
typedef int socket_t;
#define SOCKET_ERROR_VALUE -1
#define SOCKET_CLOSE(s) close(s)
#endif

#include <string>
#include <iostream>

class SocketWrapper
{
public:
    // Initialize socket library (needed for Windows)
    static bool initialize()
    {
#ifdef _WIN32
        WSADATA wsaData;
        return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
        return true;
#endif
    }

    // Clean up socket library (needed for Windows)
    static void cleanup()
    {
#ifdef _WIN32
        WSACleanup();
#endif
    }

    // Create a socket
    static socket_t createSocket()
    {
        return socket(AF_INET, SOCK_STREAM, 0);
    }

    // Set socket options
    static bool setReuseAddr(socket_t sock)
    {
        int opt = 1;
#ifdef _WIN32
        return setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) == 0;
#else
        return setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0;
#endif
    }

    // Bind socket to address
    static bool bindSocket(socket_t sock, int port)
    {
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        return bind(sock, (struct sockaddr *)&address, sizeof(address)) == 0;
    }

    // Listen for connections
    static bool listenSocket(socket_t sock, int backlog)
    {
        return listen(sock, backlog) == 0;
    }

    // Accept a connection
    static socket_t acceptConnection(socket_t sock, struct sockaddr_in *addr, socklen_t *addrSize)
    {
        return accept(sock, (struct sockaddr *)addr, addrSize);
    }

    // Send data
    static int sendData(socket_t sock, const char *data, int length, int flags = 0)
    {
        return send(sock, data, length, flags);
    }

    // Receive data
    static int receiveData(socket_t sock, char *buffer, int length, int flags = 0)
    {
        return recv(sock, buffer, length, flags);
    }

    // Close a socket
    static void closeSocket(socket_t sock)
    {
        if (sock != SOCKET_ERROR_VALUE)
        {
            SOCKET_CLOSE(sock);
        }
    }

    // Get the last error
    static std::string getLastError()
    {
#ifdef _WIN32
        int error = WSAGetLastError();
        char buffer[256];
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, buffer, sizeof(buffer), NULL);
        return std::string(buffer);
#else
        return std::string(strerror(errno));
#endif
    }
};

#endif // SOCKET_WRAPPER_H