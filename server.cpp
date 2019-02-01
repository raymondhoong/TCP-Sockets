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

int listen_sd = 0;
int client_socket[MAX_CLIENTS] = {0};

void close_and_check(int fd) {
    if (close(fd) < 0) {
        cout << "ERROR: close() call failed" << endl;
        exit(EXIT_FAILURE);
    }
}

void close_sockets() {
    close_and_check(listen_sd);
    cout << "Listening socket (" << listen_sd << ") closed" << endl;
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_socket[i] != 0) {
            close_and_check(client_socket[i]);
            cout << "Client socket (" << client_socket[i] << ") closed" << endl;
        }
        else
            break;
    }
}

void quit_sigHandler(int signum) {
    cout << "Server has successfully exited from a SIGQUIT signal" << endl;
    close_sockets();
    exit(EXIT_SUCCESS);
}

void term_sigHandler(int signum) {
    cout << "Server has successfully exited from a SIGTERM signal" << endl;
    close_sockets();
    exit(EXIT_SUCCESS);
}

void set_nonblocking(int sock)
{
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        cerr << "ERROR: fcntl() call #1 failed" << endl;
        close_sockets();
        exit(EXIT_FAILURE);
    }
    
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        cerr << "ERROR: fcntl() call #2 failed" << endl;
        close_sockets();
        exit(EXIT_FAILURE);
    }
}

void setup_connection(int port) {
    // Declare variables
    int opt;
    struct sockaddr_in address;
    
    // Create listening socket
    listen_sd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sd < 0) {
        cerr << "ERROR: socket() call failed" << endl;
        exit(EXIT_FAILURE);
    }
    
    // Set master socket to allow multiple connections
    opt = 1;
    if (setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        cerr << "ERROR: setsockopt() call failed" << endl;
        exit(EXIT_FAILURE);
    }
    
    // Set socket to non-blocking
    set_nonblocking(listen_sd);
    
    // Initialize socket struct
    bzero((char*)&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    // Bind master socket to hostname and port number
    if (bind(listen_sd,(struct sockaddr*)&address,sizeof(address)) < 0) {
        cerr << "ERROR: bind() call failed" << endl;
        exit(EXIT_FAILURE);
    }
    
    // Listen for maximum of 5 incoming connections
    if (listen(listen_sd, 5) < 0) {
        cerr << "ERROR: listen() call failed" << endl;
        exit(EXIT_FAILURE);
    }
    else
        cout << "Server listening for connections..." << endl;
}

int main(int argc, char *argv[])
{
    // Declare variables
    int port_num, new_sock, count, live_sockets;
    int rc, bytes_read, i, sd, max_sd;
    int socket_ID[MAX_CLIENTS] = {0};
    
    char buf[PACKET_SIZE + 1];
    struct sockaddr_in client_addr;
    struct timeval timeout;
    
    string dir_name, dir_path, full_path;
    socklen_t client_size;
    fd_set readfds;
    ofstream cfile;
    DIR* dir;
    
    // Check proper number of command-line arguments
    if (argc != 3)
    {
        cerr << "ERROR: Invalid number of arguments - Usage: ./server <PORT> <FILE-DIR>" << endl;
        exit(EXIT_FAILURE);
    }
    
    // Check for valid port number argument
    port_num = atoi(argv[1]);
    if (port_num < 1024 || port_num > 65535)
    {
        cerr << "ERROR: Port argument must be a number between 1024 and 65535." << endl;
        exit(EXIT_FAILURE);
    }
    
    // Check if directory exists or create it
    dir_name = argv[2];
    dir = opendir(argv[2]);
    
    if (dir) {}                  // Directory already exists
    else if (ENOENT == errno) {  // Create new directory
        if (mkdir(argv[2], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) <  0) {
            cerr << "ERROR: mkdir() call failed" << endl;
            exit(EXIT_FAILURE);
        }
    }
    else {
        cerr << "ERROR: opendir() call failed" << endl;
        exit(EXIT_FAILURE);
    }
    
    // Register SIGTERM and SIGQUIT with handlers
    signal(SIGQUIT, quit_sigHandler);
    signal(SIGTERM, term_sigHandler);
    
    // Set up server connection
    setup_connection(port_num);
    
    // Intialize variables
    count = 1;
    live_sockets = 0;
    client_size = sizeof(client_addr);
    
    // Initialize directory path string
    dir_path += "./";
    dir_path += dir_name;
    dir_path += "/";
    
    // Intialize timeval struct to 15 seconds
    timeout.tv_sec = 15;
    timeout.tv_usec = 0;
    
    fd_set exceptfds;
    
    // Loop waiting for incoming connections or incoming data from connected sockets
    while(TRUE) {
        // Clear socket set
        FD_ZERO(&readfds);
        FD_ZERO(&exceptfds);
        
        // Add master socket to set
        FD_SET(listen_sd, &readfds);
        max_sd = listen_sd;
        
        // Add child sockets to set
        for (i = 0; i < MAX_CLIENTS; i++) {
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
        rc = select(max_sd + 1, &readfds, NULL, NULL, &timeout);
        if (rc < 0) {
            cerr << "ERROR: select() call failed" << endl;
            exit(EXIT_FAILURE);
        }
        if (rc == 0 && live_sockets != 0) {
            cout << "All connections timed out" << endl;
            //exit(EXIT_FAILURE);
        }
            
        // Check for incoming connections on master socket
        if (FD_ISSET(listen_sd, &readfds)) {
            // Accept new connection
            new_sock = accept(listen_sd, (struct sockaddr*)&client_addr, &client_size);
            if (new_sock < 0) {
                cerr << "ERROR: accept() call failed" << endl;
                exit(EXIT_FAILURE);
            }
            else {
                cout << "New client connection " << count << " accepted" << endl;
                live_sockets++;
                cout << "Current number of sockets: " << live_sockets << endl;
            }
            
            // Set client socket to non-blocking
            set_nonblocking(new_sock);

            // Add new socket to array of sockets
            for (i = 0; i < MAX_CLIENTS; i++) {
                // Find next empty position
                if (client_socket[i] == 0) {
                    client_socket[i] = new_sock;
                    socket_ID[i] = count;
                    count++;
                    break;
                }
            }
        }
            
        // Check for I/O operations from other sockets
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (client_socket[i] != 0) {
                sd = client_socket[i];

                if (FD_ISSET(sd, &readfds)) {
                    // Create full path name for file
                    full_path += dir_path;
                    full_path += to_string(socket_ID[i]);
                    full_path += ".file";
                    
                    // Open file or create it
                    cfile.open(full_path, fstream::app);
                    if (!cfile.is_open()) {
                        cerr << "ERROR: ofstream.open() call failed" << endl;
                        exit(EXIT_FAILURE);
                    }
                    
                    // Read incoming message or close socket
                    bytes_read = read(sd, buf, PACKET_SIZE);
                    if (bytes_read < 0) {
                        cerr << "ERROR: read() call failed" << endl;
                        exit(EXIT_FAILURE);
                    }
                    else if (bytes_read == 0) {  // Client closed connection, finished transfer
                        cout << "Client " << socket_ID[i] << " successfully finished, socket (" << client_socket[i] << ") closed" << endl;
                        close_and_check(sd);
                        cfile.close();
                        cfile.clear();
                        client_socket[i] = 0;
                        live_sockets--;
                        cout << "Current number of sockets: " << live_sockets << endl;
                    }
                    else {                       // Write client data to file
                        buf[bytes_read] = '\0';
                        //cout << "Client " << socket_ID[i] << ": bytes read = " << bytes_read << endl;
                        cfile << buf;
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
