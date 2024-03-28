#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
int main(int argc, char **argv)
{
    int sockFd, port = 20000;
    if (argc > 1)
        port = atoi(argv[1]);
    // Create a socket
    if ((sockFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("unable to create a socket :(");
        exit(1);
    }
    // Bind the socket to the port
    struct sockaddr_in serverAddr, clientAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr=INADDR_ANY;
    socklen_t sockLen = sizeof(serverAddr), clilen;
    if (bind(sockFd, (const struct sockaddr *)&serverAddr, sockLen) < 0)
    {
        perror("Unable to bind the socket to given address :(");
        exit(1);
    }
    printf("Server is running at port:%d\n", port);
    char buff[1];
    while (1)
    {
        recvfrom(sockFd, buff, 1, 0, (struct sockaddr *)&clientAddr, &clilen);
        printf("Data recieved from client %c\n",*buff);
    }
    close(sockFd);

    return 0;
}