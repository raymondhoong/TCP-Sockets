#include <arpa/inet.h>
#include <errno.h>
#include <iostream>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
using namespace std;

#define TRUE 1
#define FALSE 0
#define PACKET_SIZE 1024

int sockfd, filefd;
struct timeval timeout;

void exit_processing(int success) {
    close(filefd);
    close(sockfd);
    
    if (success)
        exit(EXIT_SUCCESS);
    else
        exit(EXIT_FAILURE);
}

void connect_to_server(const char *host, int port) {
    // Declare variables
    int flags, rc, optval;
    struct hostent *server;
    struct sockaddr_in address;
    fd_set writefds;
    socklen_t optlen;
    
    // Create socket for client
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cerr << "ERROR: " << strerror(errno) << " - socket() call failed" << endl;
        close(filefd);
        exit(EXIT_FAILURE);
    }
    
    // Check for valid hostname
    server = gethostbyname(host);
    if (server == NULL) {
        cerr << "ERROR: " << strerror(errno) << " -  gethostbyname() call failed" << endl;
        exit_processing(FALSE);
    }
    
    // Assign information for server address
    bzero((char *)&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    bcopy((char *)server->h_addr, (char *)&address.sin_addr.s_addr, server->h_length);
    
    // Set socket to non-blocking
    flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        cerr << "ERROR: " << strerror(errno) << " - fcntl() call failed" << endl;
        exit_processing(FALSE);
    }
    
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        cerr << "ERROR: " << strerror(errno) << " - fcntl() call failed" << endl;
        exit_processing(FALSE);
    }
    
    // Attempt to connect the socket with timeout
    rc = connect(sockfd, (struct sockaddr *)&address, sizeof(address));
    if (rc < 0) {
        if (errno == EINPROGRESS) {
            cout << "Attempting to connect to server..." << endl;
            do {
                FD_ZERO(&writefds);
                FD_SET(sockfd, &writefds);
                
                rc = select(sockfd + 1, NULL, &writefds, NULL, &timeout);
                if (rc < 0 && errno != EINTR) {
                    cout << "ERROR: " << strerror(errno) << " - select() call failed" << endl;
                    exit_processing(FALSE);
                }
                else if (rc > 0) {
                    // Socket selected for write
                    optlen = sizeof(int);
                    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)(&optval), &optlen) < 0) {
                        cout << "ERROR: " << strerror(errno) << " - getsockopt() call failed" << endl;
                        exit_processing(FALSE);
                    }
                    
                    // Check if connection was successful
                    if (optval) {
                        cout << "ERROR: Delayed connection attempt failed" << endl;
                        exit_processing(FALSE);
                    }
                    else {
                        cout << "Connection to server established" << endl;
                    }
                    break;
                }
                else {
                    cout << "ERROR: Connection attempt timed out" << endl;
                    exit_processing(FALSE);
                }
            } while(TRUE);
        }
        else {
            cout << "ERROR: " << strerror(errno) << " - connect() call failed" << endl;
            exit_processing(FALSE);
        }
    }
    
    // Set socket back to blocking mode
    /*if (fcntl(sockfd, F_GETFL, NULL) < 0) {
        cerr << "ERROR: " << strerror(errno) << " - fcntl() call #3 failed" << endl;
        exit_processing(FALSE);
    }
    
    if (fcntl(sockfd, F_SETFL, flags & ~O_NONBLOCK) < 0) {
        cerr << "ERROR: " << strerror(errno) << " - fcntl() call #4 failed" << endl;
        exit_processing(FALSE);
    }*/
}

int main(int argc, char *argv[]) {
    // Declare variables
    int bytes_read, bytes_sent;
    char *sub;
    char buffer[PACKET_SIZE + 1];
    
    // Check proper number of command-line arguments
    if (argc != 4) {
        cerr << "ERROR: Invalid number of arguments - Usage: ./client <HOSTNAME-OR-IP> <PORT> <FILENAME>" << endl;
        exit(EXIT_FAILURE);
    }
    
    // Check for valid port number argument
    int port_num = atoi(argv[2]);
    if (port_num < 1024 || port_num > 65535) {
        cerr << "ERROR: Port argument must be a number between 1024 and 65535." << endl;
        exit(EXIT_FAILURE);
    }
    
    // Open specified file
    filefd = open(argv[3], O_RDONLY);
    if (filefd < 0) {
        cerr << "ERROR: " << strerror(errno) << " - open() call failed" << endl;
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    // Set timeout to 15 secs
    timeout.tv_sec = 15;
    timeout.tv_usec = 0;
    
    // Attempt TCP connection to server
    connect_to_server(argv[1], port_num);
        
    // Transfer file over socket connection
    cout << "Attemping to transfer file..." << endl;
    
    while (TRUE) {
        // Read data into buffer
        bytes_read = read(filefd, buffer, sizeof(buffer));
        
        if (bytes_read == 0)
            break;
        if (bytes_read < 0) {
            cerr << "ERROR: " << strerror(errno) << " - read() call failed" << endl;
            exit_processing(FALSE);
        }
        
        // Write data to socket
        sub = buffer;
        while (bytes_read > 0) {
            bytes_sent = send(sockfd, sub, bytes_read, MSG_NOSIGNAL);
            if (bytes_sent < 0 && errno != EAGAIN) {
                cerr << "ERROR: " << strerror(errno) << " - write() call failed" << endl;
                exit_processing(FALSE);
            }
            bytes_read -= bytes_sent;
            sub += bytes_sent;
        }
    }
    
    cout << "File succesfully transfered" << endl;
    exit_processing(TRUE);
}
