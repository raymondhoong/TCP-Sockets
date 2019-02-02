# CS118 Project 1

## Makefile

This provides a couple make targets for things.
By default (all target), it makes the `server` and `client` executables.

It provides a `clean` target, and `tarball` target to create the submission file as well.

You will need to modify the `Makefile` to add your userid for the `.tar.gz` turn-in at the top of the file.

## Academic Integrity Note

You are encouraged to host your code in private repositories on [GitHub](https://github.com/), [GitLab](https://gitlab.com), or other places.  At the same time, you are PROHIBITED to make your code for the class project public during the class or any time after the class.  If you do so, you will be violating academic honestly policy that you have signed, as well as the student code of conduct and be subject to serious sanctions.

## Provided Files

`server.cpp` and `client.cpp` are the entry points for the server and client part of the project.

## TODO

Raymond Hoong
UCLA ID: 904604520

For my client application, I made my single socket non-blocking. Using the select( ) function, I was able to check for a connection attemp timeout set at 15 seconds. This is set in a do-while loop, which keeps testing if a connection has been made with the server within the timeout span. Next the file, that is to be transferred, is opened and read into a buffer. Whatever read into the buffer is then sent through the socket to the server using the send( ) call. This is all done in a infinite-while loop, which will only break once the number of bytes read from the file result in 0. This will indicate the EOF of the file. 

For my server application, I decided to implement the design of handling multiple clients using the select( ) call instead of threads. A listening socket is created first and set to non-blocking. Then in a infinite-while loop, the select( ) call is used with the fd_set readfds. An array client_socket[i] holds all the valid client sockets. It's value is the file descriptor for that socet. A value of 0 means that it isn't in use. When the listening socket has activity, indicated by being in the readfds when probed by select( ), it means that there is a new incoming connection waiting to be accepted. From there, the new client connection is added to client_socket[i] and given its ID number, which is held in socket_ID[i]. If any of the client sockets are found in readfds, this means that there is a pending I/O operation to be performed on that socket, which is carried out. The file is then written using ofstream and a value of 0 returned by bytes read means the connection has closed gracefully. To implement the timeout, I maintained an array last_act[i] to record the times of the last time a socket has performed an I/O operation. Then at each iteration of the loop, I compare the time from last_act[i] to the time of the current I/O operation. If it's greater than 15 seconds, I shut down the connection and overwrite the file with "ERROR" 

The problem I ran into was trying to find a way to implement the timeout feature for the server application. I first attempted to use select() and its timeout option to do so, but realized that it was not possible, or at least easily done. This is because select() will only return 0 to indicate if all the descriptors in its set has timed out. It doesn't indicate whether a particular connection timed out or not. Thus, I used a more blunt approach and just recorded the times that each client socket had an I/O operation. If the times between each operation was greater than 15 seconds, I treated it as a timeout error. 

I used many online tutorials on how to complete this project.  The links are pasted below for your reference. I've also referenced some code that I've written for projects done in CS111.

http://www.linuxhowtos.org/data/6/client.c
http://developerweb.net/viewtopic.php?id=3196
https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.3.0/com.ibm.zos.v2r3.hala001/orgblockasyn.htm
http://pubs.opengroup.org/onlinepubs/7908799/xsh/time.h.html
http://man7.org/linux/man-pages/man3/gethostbyname.3.html
https://stackoverflow.com/questions/14378957/detecting-a-timed-out-socket-from-a-set-of-sockets
http://pubs.opengroup.org/onlinepubs/7908799/xsh/difftime.html
