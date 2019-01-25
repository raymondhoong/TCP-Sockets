#include <arpa/inet.h>
#include <errno.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

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
    
    // Store filename argument
    string file_name = argv[3];
    
    // Create a socket point
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        cerr << "ERROR: Failed to create a socket point for the client... exiting program" << endl;
        exit(EXIT_FAILURE);
    }
    
    // Retrieve the hostname of the server to connect
    struct hostent *server;
    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        cerr << "ERROR: Failed in retrieving host name... exiting program" << endl;
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
        cerr << "ERROR: Failed in connecting to server... exiting program" << endl;
        exit(EXIT_FAILURE);
    }
    else
        cout << "Connection succcess!!" << endl;
    
    exit(EXIT_SUCCESS);
}
