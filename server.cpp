#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

#define TRUE 1
#define FALSE 0
#define MAX_CLIENTS 30
#define PACKET_SIZE 1024

void sigQuit_handler(int signum)
{
    cout << "Program successfully terminated by SIGQUIT" << endl;
    exit(EXIT_SUCCESS);
}

void sigTerm_handler(int signum)
{
    cout << "Program successfully terminated by SIGTERM" << endl;
    exit(EXIT_SUCCESS);
}

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
    
    // Check if directory exists or create it
    string dir_name = argv[2];
    DIR* dir = opendir(argv[2]);
    if (dir) {/* Do nothing */}
    else if (ENOENT == errno)  // Directory doesn't exist
    {
        const int dir_err = mkdir(argv[2], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (dir_err == -1)
        {
            cerr << "ERROR: Failed to create new directory " << argv[2] << "... exiting program" << endl;
            exit(EXIT_FAILURE);
        }
    }
    else // Failed from an invalid reason
    {
        cerr << "ERROR: Failed to check if the directory " << argv[2] << "exists... exiting program" << endl;
        exit(EXIT_FAILURE);
    }
    
    // Register SIGTERM and SIGQUIT with handlers
    signal(SIGQUIT, sigQuit_handler);
    signal(SIGTERM, sigTerm_handler);
    
    // Create socket point
    int listen_sd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sd == -1)
    {
        cerr << "ERROR: Failed to create a socket point for the server... exiting program" << endl;
        exit(EXIT_FAILURE);
    }
    
    // Set master socket to allow multiple connections
    int opt = 1;
    if (setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) == -1)
    {
        cerr << "ERROR: Failed to configure the socket... exiting program" << endl;
        exit(EXIT_FAILURE);
    }
    
    // Make socket non-blocking
    int status = fcntl(listen_sd, F_SETFL, O_NONBLOCK);
    if (status == -1)
    {
        cerr << "ERROR: Failed to set socket to non-blocking... exiting program" << endl;
        exit(EXIT_FAILURE);
    }
    
    // Initialize socket struct
    struct sockaddr_in serv_addr;
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port_num);
    
    // Bind master socket to hostname and port number
    if (bind(listen_sd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1)
    {
        cerr << "ERROR: Failed to bind the socket... exiting program" << endl;
        exit(1);
    }
    
    // Listen for maximum of 5 incoming connections
    if (listen(listen_sd, 5) == -1)
    {
        cerr << "ERROR: Failed to listen for incoming connections... exiting program" << endl;
        exit(1);
    }
    
    struct sockaddr_in client_addr;
    socklen_t client_size = sizeof(client_addr);
    
    int new_sock;
    int client_socket[MAX_CLIENTS] = {0};
    int socket_ID[MAX_CLIENTS] = {0};
    int count = 1;
    int live_sockets = 0;
    int retval;
    int bytes_read;
    int i, sd, max_sd;
    
    char buf[PACKET_SIZE + 1];  // 1KiB
    fd_set readfds;  // Set of socket descriptors
    struct timeval timeout;
    ofstream cfile;
    
    // Initialize directory path string
    string dir_path, full_path;
    dir_path += "./";
    dir_path += dir_name;
    dir_path += "/";
    
    // Intialize timeval struct to 15 seconds
    timeout.tv_sec = 15;
    timeout.tv_usec = 0;
    
    // Loop waiting for incoming connections or incoming data from connected sockets
    while(TRUE)
    {
        // Clear socket set
        FD_ZERO(&readfds);
        
        // Add master socket to set
        FD_SET(listen_sd, &readfds);
        max_sd = listen_sd;
        
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
        }
            
        // Call select() and wait for timeout to complete
        retval = select(max_sd + 1, &readfds, NULL, NULL, &timeout);
        if (retval < 0)
        {
            cerr << "ERROR: Function select() failed unexpectedly ... exiting program" << endl;
            exit(EXIT_FAILURE);
        }
        if (retval == 0 && live_sockets != 0)
        {
            cout << "Connection timed out" << endl;
            exit(EXIT_FAILURE);
        }
            
        // Check for incoming connections on master socket
        if (FD_ISSET(listen_sd, &readfds))
        {
            // Accept new connection
            new_sock = accept(listen_sd, (struct sockaddr*)&client_addr, &client_size);
            if (new_sock == -1)
            {
                cerr << "ERROR: Failed to accept new connection... exiting program" << endl;
                exit(EXIT_FAILURE);
            }
            else
                live_sockets++;
            
            // Make socket non-blocking
            /*status = fcntl(new_sock, F_SETFL, O_NONBLOCK);
            if (status == -1)
            {
                cerr << "ERROR: Failed to set socket to non-blocking... exiting program" << endl;
                exit(EXIT_FAILURE);
            }*/

            // Add new socket to array of sockets
            for (i = 0; i < MAX_CLIENTS; i++)
            {
                // Find next empty position
                if (client_socket[i] == 0)
                {
                    client_socket[i] = new_sock;
                    socket_ID[i] = count;
                    count++;
                    break;
                }
            }
        }
            
        // Check for I/O operations from other sockets
        for (i = 0; i < MAX_CLIENTS; i++)
        {
            if (client_socket[i] != 0)
            {
                //cout << "Socket client #: " << i << endl;
                sd = client_socket[i];

                if (FD_ISSET(sd, &readfds));
                {
                    // Create full path name for file
                    full_path += dir_path;
                    full_path += to_string(socket_ID[i]);
                    full_path += ".file";
                    
                    // Open file or create it
                    cfile.open(full_path, fstream::app);
                    if (!cfile.is_open())
                    {
                        cerr << "ERROR: Failed to open file from client... exiting program" << endl;
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        cout << "File " << full_path << " opened" << endl;
                    }
                    
                    // Read incoming message or close socket
                    bytes_read = read(sd, buf, PACKET_SIZE);
                    
                    if (bytes_read < 0)
                    {
                        cerr << "ERROR: Failed to read data from socket... exiting program" << endl;
                        exit(EXIT_FAILURE);
                    }
                    else if (bytes_read == 0)
                    {
                        close(sd);
                        cfile.close();
                        cfile.clear();
                        cout << "Client socket " << socket_ID[i] << " closed." << endl;
                        client_socket[i] = 0;
                        live_sockets--;
                        cout << "FINISHED" << endl;
                    }
                    else
                    {
                        buf[bytes_read] = '\0';
                        cfile << buf;
                        cout << "Client socket " << socket_ID[i] << " written for." << endl;
                        cfile.close();
                        cfile.clear();
                    }
                }
                    
                // Reset the path name
                full_path.clear();
            }
        }
    }
}
