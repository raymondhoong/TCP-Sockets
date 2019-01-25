#include <arpa/inet.h>
#include <errno.h>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

using namespace std;

#define TRUE 1
#define FALSE 0
#define MAX_CLIENTS 30

int main(int argc, char *argv[])
{
    // Check proper number of command-line arguments
    if (argc != 3)
    {
        cerr << "ERROR: Invalid number of arguments - Usage: ./server <PORT> <FILE-DIR>" << endl;
        exit(EXIT_FAILURE);
    }
    
    // Check for valid port number argument
    int port_num = atoi(argv[1]);
    if (port_num < 1024 || port_num > 65535)
    {
        cerr << "ERROR: Port argument must be a number between 1024 and 65535." << endl;
        exit(EXIT_FAILURE);
    }
    
    // Store the file directory argument
    string dir_name = argv[2];
    
    // Create socket point
    int master_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (master_sock == -1)
    {
        cerr << "ERROR: Function socket() failed unexpectedly for master_sock." << endl;
        exit(EXIT_FAILURE);
    }
    
    // Set master socket to allow multiple connections
    int opt = 1;
    if (setsockopt(master_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) == -1)
    {
        cerr << "ERROR: Function setsockopt() failed unexpectedly for master_sock." << endl;
        exit(EXIT_FAILURE);
    }
    
    // Initialize socket struct
    struct sockaddr_in serv_addr;
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(port_num);
    
    // Bind master socket to hostname and port number
    if (bind(master_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1)
    {
        cerr << "ERROR: Function bind() failed unexpectedly. for master_sock." << endl;
        exit(1);
    }
    
    // Listen for maximum of 5 incoming connections
    if (listen(master_sock, 5) == -1)
    {
        cerr << "ERROR: Function listen() failed unexpectedly." << endl;
        exit(1);
    }
    
    // Accept incoming connection
    struct sockaddr_in client_addr;
    socklen_t client_size = sizeof(client_addr);
    
    int new_sock;
    int client_socket[MAX_CLIENTS] = {0};
    int activity;
    int bytes_read;
    int i, sd, max_sd;
    
    char buf[1025];  // 1KiB
    fd_set readfds;  // Set of socket descriptors
    
    // Initialize client_socket
    
    while(TRUE)
    {
        // Clear socket set
        FD_ZERO(&readfds);
        
        // Add master socket to set
        FD_SET(master_sock, &readfds);
        max_sd = master_sock;
        
        // Add child sockets to set
        for (i = 0; i < MAX_CLIENTS; i++)
        {
            // Assign socket descriptor
            sd = client_socket[i];
            
            // If valid, add descriptor to list
            if (sd > 0)
                FD_SET(sd, &readfds);
            
            // Get highest file descriptor number
            if (sd > max_sd)
                max_sd = sd;
            
            // Wait indefinitely for activity on one of the sockets
            // *** Should timeout after 15 seconds
            activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
            if (activity < 0)
            {
                cerr << "ERROR: Function select() failed unexpectedly... Program exited" << endl;
                exit(EXIT_FAILURE);
            }
            
            // Check for incoming connections on master socket
            if (FD_ISSET(master_sock, &readfds))
            {
                // Accept new connection
                new_sock = accept(master_sock, (struct sockaddr*)&client_addr, &client_size);
                if (new_sock == -1)
                {
                    cerr << "ERROR: Function accept() failed unexpectedly... Program exited" << endl;
                    exit(EXIT_FAILURE);
                }
                
                // Add new socket to array of sockets
                for (i = 0; i < MAX_CLIENTS; i++)
                {
                    // Find next empty position
                    if (client_socket[i] == 0)
                    {
                        client_socket[i] = new_sock;
                        break;
                    }
                }
            }
            
            // Check for I/O operations from other sockets
            for (i = 0; i < MAX_CLIENTS; i++)
            {
                if (client_socket[i] != 0)
                {
                    sd = client_socket[i];
                    
                    if (FD_ISSET(sd, &readfds));
                    {
                        // Read incoming message or close socket
                        bytes_read = read(sd, buf, 1024);
                        if (bytes_read < 0)
                        {
                            cerr << "ERROR: Function read() failed unexpectedly... Program exited" << endl;
                            exit(EXIT_FAILURE);
                        }
                        else if (bytes_read == 0)
                        {
                            close(sd);
                            client_socket[i] = 0;
                        }
                        else
                        {
                            buf[bytes_read] = '\0';
                            cout << buf << endl;
                        }
                    }
                }
            }
        }
    }
}
