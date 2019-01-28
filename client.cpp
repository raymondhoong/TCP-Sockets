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

#define TRUE 1
#define FALSE 0
#define PACKET_SIZE 1024

using namespace std;

int main(int argc, char *argv[])
{
    // Check proper number of command-line arguments
    if (argc != 4)
    {
        cerr << "ERROR: Invalid number of arguments - Usage: ./client <HOSTNAME-OR-IP> <PORT> <FILENAME>" << endl;
        exit(EXIT_FAILURE);
    }
    
    // Check for valid port number argument
    int port_num = atoi(argv[2]);
    if (port_num < 1024 || port_num > 65535)
    {
        cerr << "ERROR: Port argument must be a number between 1024 and 65535." << endl;
        exit(EXIT_FAILURE);
    }
    
    // Create a socket point
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        cerr << "ERROR: Failed to create a socket point for the client... exiting program" << endl;
        exit(EXIT_FAILURE);
    }
    
    // Make socket non-blocking
    /*int status = fcntl(sockfd, F_SETFL, O_NONBLOCK);
    if (status == -1)
    {
        cerr << "ERROR: Failed to set socket to non-blocking... exiting program" << endl;
        exit(EXIT_FAILURE);
    }*/
    
    // Retrieve the hostname of the server to connect
    struct hostent *server;
    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        cerr << "ERROR: Failed to retrieve host " << argv[1] << "... exiting program" << endl;
        exit(EXIT_FAILURE);
    }
    
    // Set the information for the server address
    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_num);
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    
    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        cerr << "ERROR: Failed to connect to server on port " << port_num << "... exiting program" << endl;
        exit(EXIT_FAILURE);
    }
    
    // Open the specified file
    int fp = open(argv[3], O_RDONLY);
    if (fp < 0)
    {
        cerr << "ERROR: Failed to open file " << argv[3] << "... exiting program" << endl;
        exit(EXIT_FAILURE);
    }
    
    // Transfer file over socket connection
    char buffer[PACKET_SIZE + 1];
    int bytes_read, bytes_written;
    char *sub;
    
    while (TRUE)
    {
        // Read data into buffer
        bytes_read = read(fp, buffer, sizeof(buffer));
        
        if (bytes_read == 0)
            break;
        if (bytes_read < 0)
        {
            cerr << "ERROR: Failed to read data from file " << argv[3] << "... exiting program" << endl;
            exit(EXIT_FAILURE);
        }
        
        // Write data to socket
        sub = buffer;
        while (bytes_read > 0)
        {
            bytes_written = write(sockfd, sub, bytes_read);
            if (bytes_written <= 0)
            {
                cerr << "ERROR: Failed to write file data to socket... exiting program" << endl;
                exit(EXIT_FAILURE);
            }
            bytes_read -= bytes_written;
            sub += bytes_written;
        }
    }

    close(fp);
    close(sockfd);
    exit(EXIT_SUCCESS);
}
