#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#define _GNU_SOURCE
#include <signal.h>
#include <poll.h>
int main(int argc, char **argv)
{
    // Create a socket
    int sockfd, serverPort = 20000;
    char *server_ip;
    if (argc > 1)
    {
        server_ip=(char*)malloc(strlen(argv[1])*sizeof(char));
        strcpy(server_ip,argv[1]);
        printf("Connecting to server with IP: %s\n",server_ip);
    }
    else
    {
        printf("Usage ./client <server_ip>");
        return 0;
    }
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Unable tot create a socket :(");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    inet_aton(server_ip, &serverAddr.sin_addr);
    socklen_t sockLen = sizeof(serverAddr);
    char *message = "PING: ";
    // char buff[100];
    // for (int i = 0; i < 100; i++)
    // {
    //     buff[i] = '\0';
    // }
    // Polling definition
    struct pollfd pollInfo;
    pollInfo.fd = sockfd;
    pollInfo.events = POLLIN;
    int retryCount = 0;
    int iter = 0;
    char buff[1] = "a";
    while (iter < 1000)
    {
        // Send the ping message to the server
        sendto(sockfd, buff, 1, 0, (const struct sockaddr *)&serverAddr, sockLen);
        printf("Sent UDP packet data: %c\n", *buff);
        (*buff)++;
        (*buff) = (*buff) % ('z' + 1);
        if (*buff == 0)
            *buff = 'a';
        iter++;
        sleep(3);
    }
    close(sockfd);

    return 0;
}