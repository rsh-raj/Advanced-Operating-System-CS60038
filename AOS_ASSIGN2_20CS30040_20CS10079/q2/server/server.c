#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>
#define _GNU_SOURCE
#include <signal.h>
#include <poll.h>
#include <pthread.h>
#define SLEEP_FACTOR 12
char free_threads[5];
typedef struct thread_arg
{
    int client_fd;
    int id;
    int sleep_duration;
    struct sockaddr_in clientAddr;
} thread_args;
pthread_mutex_t mutex_lock;
void *runner(void *param)
{
    thread_args *args = (thread_args *)param;
    int id = args->id;
    int sleep_duration = args->sleep_duration;
    int sockfd = args->client_fd;
    struct sockaddr_in clientAddr = args->clientAddr;
    printf("Current packet is being processed by thread_id %d and processing time is %d\n", id, SLEEP_FACTOR*sleep_duration);
    int x = sleep(SLEEP_FACTOR* sleep_duration);
    pthread_mutex_lock(&mutex_lock);
    char buf[1] = "1";
    struct sockaddr_in lbAddr;
    lbAddr.sin_addr.s_addr = INADDR_ANY;
    lbAddr.sin_family = AF_INET;
    lbAddr.sin_port = htons(8080);
    socklen_t sockLen = sizeof(lbAddr);
    sendto(sockfd, buf, 1, 0, (const struct sockaddr *)&lbAddr, sockLen);
    printf("The thread_id %d has successfully processed the packet and slept for %d second.\n", id,SLEEP_FACTOR*sleep_duration);
    free_threads[id] = 1;
    pthread_mutex_unlock(&mutex_lock);
    pthread_exit(0);
}
int main(int argc, char **argv)
{
    pthread_mutex_init(&mutex_lock, NULL);
    pthread_t tid[6];
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    for (int i = 0; i < 6; i++)
        free_threads[i] = 1; // mark all thread as free
    int port = 5051;
    if (argc > 1)
    {
        port = atoi(argv[1]);
    }
    else
    {
        printf("Usage ./server <port>");
        return 0;
    }
    int sockfd, newSockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Unable to create a socket :(");
        exit(0);
    }
    struct sockaddr_in serveAddr, clientAddr;
    serveAddr.sin_addr.s_addr = INADDR_ANY;
    serveAddr.sin_family = AF_INET;
    serveAddr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr *)&serveAddr, sizeof(serveAddr)) < 0)

    {
        perror("Unable to bind the socket to local address :(");
        exit(0);
    }
    int cliLen = sizeof(clientAddr);
    char buff[1];
    printf("Server is running on port: %d\n", port);
    socklen_t clilen;

    int iter = 0;

    while (1)
    {

        recvfrom(sockfd, buff, 1, 0, (struct sockaddr *)&clientAddr, &clilen);
        printf("Recieved UDP packet data: %c\n", *buff);
        // char buf[1] = {'1'};
        // socklen_t sockLen = sizeof(serveAddr);
        // if (sendto(sockfd, buf, 1, 0, (const struct sockaddr *)&clientAddr, sockLen) < 0)
        // {
        //     perror("send failed\n");
        // }
        // close(sockfd);
        // break;
        int id = 0;
        while (id < 6)
        {
            if (free_threads[id])
                break;
            id++;
        }
        if (id == 5)
        {
            printf("Some error occured, a data packet is recieved but there are no free threads\n");
            return 0;
        }
        free_threads[id] = 0;
        thread_args *arg = malloc(sizeof(thread_args));
        arg->client_fd = sockfd;
        arg->id = id;
        arg->sleep_duration = *buff - '0';
        arg->clientAddr = clientAddr;
        pthread_create(&tid[id], &attr, runner, (void *)arg);
        iter++;
    }

    pthread_mutex_destroy(&mutex_lock);

    return 0;
}
