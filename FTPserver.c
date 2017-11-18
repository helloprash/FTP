/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    int retVal;
    int numbytes;
    FILE *fptr;
    int flag;
    char TxBuff[2048];
    char RxBuff[2048];
    char message[2048];
    char fileName[50];
    char buffer[2048];

    memset(&hints, 0, sizeof hints);
    memset(&buffer, 0, sizeof buffer);
    memset(&RxBuff, 0, sizeof RxBuff);
    memset(&TxBuff, 0, sizeof TxBuff);
    memset(&fileName, 0, sizeof fileName);
    
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) 
	{
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
	{
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) 
	{
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
	{
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");
    
    while(1) 
    {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) 
	{
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        retVal = fork();
        if (retVal == 0)
	{ // this is the child process
            close(sockfd); // child doesn't need the listener
          
	   strcpy(TxBuff, "Welcome to File Transfer Protocol");
	   numbytes = send(new_fd, TxBuff, strlen(TxBuff), 0); 
           while(1)
           {
		 printf("Welcome to FTP\n");
		 memset(&buffer, 0, sizeof buffer);
       	         memset(&RxBuff, 0, sizeof RxBuff);
	         memset(&TxBuff, 0, sizeof TxBuff);
	         memset(&fileName, 0, sizeof fileName);
           	 
		 if((numbytes = recv(new_fd, RxBuff, sizeof(RxBuff), 0)) <= 0) // Recv operation
		 {
		       perror("Receive operation failed...");
	         }
	         RxBuff[numbytes] = '\0';
	         printf("Operation: %s\n",RxBuff);
			    
	         sscanf(RxBuff, "%s %s",buffer, fileName);
		 printf("Command:%s\n",buffer);
		 printf("fileName:%s\n",fileName);
			
		/*---------Send to Client***GET***----------*/
                if(strcasecmp(buffer,"Get") == 0)
                {
		     printf("Inside Get\n");
                     fptr = fopen(fileName, "r");	
		     if(fptr == NULL)
		     {
			 printf("%s\n",strerror(errno));
		         strcpy(TxBuff, "File does not exist");
			 if((numbytes = send(new_fd, TxBuff, strlen(TxBuff), 0)) <= 0)
			 {
			    perror("Send failed");
		         }
		     } 
		        
		      //Reading file and sending
		     flag=1;
		    while(flag)
                    {
                	memset(&message, 0, sizeof message);
                        retVal = fscanf(fptr,"%s",message) ; /* Reading from the file */
			printf("%s\n",message);
                        if (retVal > 0)
                        {
                            if((numbytes = send(new_fd, message, strlen(message), 0)) <=0)
                            {
                   	   	 perror("Send failed...");
			    }
                        }
                        else if (retVal == EOF)
                        {
			    printf("End of File\n");
                            strcpy(message, "EOF");
		            if((numbytes = send(new_fd, message, strlen(message), 0)) <=0)
                            {
                   	   	perror("Send failed...");
			    }
                            flag = 0; 
                            fclose(fptr);
			    printf("Exited..\n");
                        }
                     }
		 }
				
	        /*-------Receive from client***PUT***------*/
		else if(strcasecmp(buffer,"Put") == 0)
		{
	              printf("Put operation\n");
		      fptr = fopen(fileName, "w");	
		      if(fptr == NULL)
		      {
			      printf("File open error\n");
		      }
					
		      //Receiving loop
		      flag = 1;
		      while(flag)
		      {
		          memset(&RxBuff, 0, sizeof RxBuff);
		          if((numbytes = recv(new_fd, RxBuff, sizeof RxBuff, 0)) <= 0)
                          {
                   	        perror("Receive file failed...");
				flag=0;
	                  }
	            
	                  if(strcmp(RxBuff, "EOF") == 0)
	                  {
	            	      fclose(fptr);
			      flag = 0;
	                  }
	                  else
	                      fprintf(fptr, "%s ", RxBuff);
                      }	
		}
           }
        }
        else if (retVal < 0)
        {
             printf("fork failed \n");
        }
	printf("End of Accept loop\n");
    }
    return 0;
}
