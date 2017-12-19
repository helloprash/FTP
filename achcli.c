/*Client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "3491" // the port client will be connecting to

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int sockfd, numbytes;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int retVal ;
    char s[INET6_ADDRSTRLEN];
    char message[2048];
    char TxBuff[2048];
    char RxBuff[2048];
    char name[30];

    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    memset(&RxBuff, 0, sizeof RxBuff);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1)
        {
            perror("client: socket");
            continue;
        }


        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    memset(&name, 0, sizeof name);
    printf("Enter your name:");
    scanf("%s",name);

    numbytes = send(sockfd, name, strlen(name), 0);
    if(numbytes == -1)
	   perror("Sending name failed");

    numbytes = recv(sockfd, RxBuff, sizeof RxBuff, 0); //receive Welcome
    if (numbytes == -1)
    {
        perror("recv");
        exit(1);
    }

    RxBuff[numbytes] = '\0';
    printf("%s\n",RxBuff);

    while(1)
    {
          memset(&TxBuff, 0, sizeof TxBuff);
          memset(&RxBuff, 0, sizeof RxBuff);
          memset(&message, 0, sizeof message);
          
	  printf("You:");
          fgets(TxBuff, sizeof TxBuff, stdin);
	  numbytes = send(sockfd, TxBuff, strlen(TxBuff), 0);
	  if(numbytes == -1)
               perror("Send Failed");
    }
    close(sockfd);
    return 0;
}

                      
