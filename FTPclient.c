/*
** client.c -- a stream socket client demo
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

#define PORT "3490" // the port client will be connecting to 


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
    int sockfd;  
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    char s[INET6_ADDRSTRLEN];
    int flag;
    int retVal;
    char listOfFiles[] = "letter.txt\nhello.txt";
    char TxBuff[2048];
    char RxBuff[2048];
    char message[2048];
    char fileName[50];
    char buffer[2048];
    FILE *fptr;

    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    memset(&TxBuff, 0, sizeof TxBuff);
    memset(&RxBuff, 0, sizeof RxBuff);
    memset(&fileName, 0, sizeof fileName);
    memset(&buffer, 0, sizeof buffer);
    
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
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

    numbytes = recv(sockfd, RxBuff, sizeof RxBuff, 0);
    RxBuff[numbytes] = '\0';
    printf("%s\n",RxBuff);

    while(1)
    {    
        memset(&TxBuff, 0, sizeof TxBuff);
	memset(&RxBuff, 0, sizeof RxBuff);
	memset(&fileName, 0, sizeof fileName);
        memset(&buffer, 0, sizeof buffer);

	printf("1. List dummy\n");
	printf("2. Get fileName\n");
	printf("3. Put fileName\n");
	printf("4. Quit dummy\n");

        scanf("%s %s",TxBuff,fileName);

        printf("Operation:%s\n",buffer); 
        printf("Command:%s\n",TxBuff);
        printf("fileName:%s\n",fileName);
        
        sprintf(buffer,"%s %s",TxBuff,fileName);
        /*-----List of Files-----*/
        if(strcasecmp(TxBuff,"List") == 0)
        {
            printf("%s\n",listOfFiles);
	}
		
        /*------Receive from Server***GET***------*/
	else if(strcasecmp(TxBuff, "Get")== 0)
        {
	     if((numbytes = send(sockfd, buffer, strlen(buffer), 0)) <= 0)  // Send Operation
             {
		 perror("Send failed...");
             }
			
	     fptr = fopen(fileName,"w");
	     if(fptr == NULL)
	     {
		 printf("%s\n",strerror(errno));
	     }
	    //Receiving loop
	    flag = 1;
	     while(flag)
             {
		 memset(&RxBuff, 0, sizeof RxBuff);
		 numbytes = recv(sockfd, RxBuff, sizeof RxBuff, 0);
		 if(numbytes <= 0)
                 {
                   	perror("Receive failed...");
			flag = 0;
	         }
		 RxBuff[numbytes] = '\0';
		 printf("%s ",RxBuff);
		 printf("num:%d\n",numbytes);
	            
	         if(strcmp(RxBuff, "EOF") == 0)
	         {
	             fclose(fptr);
		     flag = 0;
		     printf("\n%s received\n",fileName);
		 }
	         else
	             fprintf(fptr, "%s", RxBuff);
	     }
         }
		 
	 /*-----Send to Server***PUT***----*/
	 else if(strcasecmp(TxBuff, "Put") == 0)
	 {
	      if((numbytes = send(sockfd, buffer, strlen(buffer), 0)) <= 0)  // Send Operation
	      {
		   perror("Send failed...");
	      }
		    
	      fptr = fopen(fileName, "r");
	      if(fptr == NULL)
	      {
		  printf("%s\n",strerror(errno));
	      }
			
	     //Reading file and sending
	      flag=1;
	      while(flag == 1)
              {
                  memset(&message, 0, sizeof message);
                  retVal = fscanf(fptr,"%c",message) ; /* Reading from the file */
                  if (retVal > 0)
                  {
		      printf("%s ",message);
                      if((numbytes = send(sockfd, message, strlen(message), 0)) <=0)
                      {
                     	  perror("Send failed...");
	              }
		      printf("num:%d\n",numbytes);
                  }
                  else if (retVal == EOF)
                  {
                      strcpy(message, "EOF");
		      if((numbytes = send(sockfd, message, strlen(message), 0)) <=0)
                      {
                   	   perror("Send failed...");
	              }
                      flag = 0;
		      printf("End of File\n"); 
                      fclose(fptr);
		      printf("\n%s sent\n",fileName);
                  }
		  sleep(1);
              }
	      printf("Exited Put...\n");
	 }
	else if(strcasecmp(TxBuff, "Quit") == 0)
	{
             break;
	}
	printf("\n");
      }
    close(sockfd);

    return 0;
}
