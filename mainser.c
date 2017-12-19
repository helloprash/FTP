#include <stdio.h> 
#include <string.h>    
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h>   
#include <arpa/inet.h>    
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h> 
    
#define PORT 8888 
    
int main()  
{  
    int opt = 1;  
    int main_socket , addrlen , new_socket , client_socket[30];
    int max_clients = 30 , activity, i , numbytes , sd;  
    int max_sd;  
    struct sockaddr_in address;  
    char buffer[1025];   
    
    addrlen = sizeof(address);  
    //set of socket descriptors 
    fd_set readfds;  
         
    char *message = "Welcome to Group Chat";  
    
    memset(&client_socket, 0, sizeof client_socket);

    //create a main socket 
    if( (main_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)  
    {  
        perror("socket failed");  
        exit(EXIT_FAILURE);  
    }  
    
    if( setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, 
          sizeof(opt)) < 0 )  
    {  
        perror("setsockopt");  
        exit(EXIT_FAILURE);  
    }  
    
    //type of socket created 
    address.sin_family = AF_INET;  
    address.sin_addr.s_addr = INADDR_ANY;  
    address.sin_port = htons( PORT );  
        
    //bind the socket to localhost port 8888 
    if (bind(main_socket, (struct sockaddr *)&address, sizeof(address))<0)  
    {  
        perror("bind failed");  
        exit(EXIT_FAILURE);  
    }  
    printf("Listener on port %d \n", PORT);  
        
    //listen to max of 3 connections from main_socket
    if (listen(main_socket, 3) < 0)  
    {  
        perror("listen");  
        exit(EXIT_FAILURE);  
    }  
        
    //accept the incoming connection 
    printf("Waiting for connections ...\n");  
        
    while(1)  
    {  
        //clear the socket set 
        FD_ZERO(&readfds);  
    
        //add main socket to set 
        FD_SET(main_socket, &readfds);  
        max_sd = main_socket;  
            
        //add child sockets to set 
        for ( i = 0 ; i < max_clients ; i++)  
        {  
            //socket descriptor 
            sd = client_socket[i];  
                
            //if valid socket descriptor then add to read list 
            if(sd > 0)  
                FD_SET( sd , &readfds);  
                
            //highest file descriptor number, need it for the select function 
            if(sd > max_sd)  
                max_sd = sd;  
        }  
    
        //wait for an activity on one of the sockets , timeout is NULL , 
        //so wait indefinitely 
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);  
      
        if ((activity < 0) && (errno!=EINTR))  
        {  
            printf("select error");  
        }  
            
        //If something happened on the main socket , 
        //then its an incoming connection 
        if (FD_ISSET(main_socket, &readfds))  
        {  
            if ((new_socket = accept(main_socket,(struct sockaddr *)&address, (socklen_t*)&addrlen))<0)  
            {  
                perror("accept");  
                exit(EXIT_FAILURE);  
            }  
            
            //inform user of socket number - used in send and receive commands 
            /*printf("New connection , socket fd is %d , ip is : %s , port : %d 
                  \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs
                  (address.sin_port));*/
	    printf("New Connection estd\n");
          
            //send new connection greeting message 
            if( send(new_socket, message, strlen(message), 0) != strlen(message) )  
            {  
                perror("send");  
            }  
                
            printf("Welcome message sent successfully\n");  
                
            //add new socket to array of sockets 
            for (i = 0; i < max_clients; i++)  
            {  
                //if position is empty 
                if( client_socket[i] == 0 )  
                {  
                    client_socket[i] = new_socket;  
                    printf("Adding to list of sockets as %d\n" , i);  
                        
                    break;  
                }  
            }  
        }  
            
        //else its some IO operation on some other socket
        for (i = 0; i < max_clients; i++)  
        {  
            sd = client_socket[i];  
                
            if (FD_ISSET( sd , &readfds))  
            {  
                //Check if it was for closing , and also read the 
                //incoming message
                if ((numbytes = recv( sd , buffer, sizeof buffer, 0)) == 0)  
                {  
                    //Somebody disconnected , get his details and print 
                    getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);  
                    printf("Host disconnected , ip %s , port %d \n" , 
                          inet_ntoa(address.sin_addr) , ntohs(address.sin_port));  
                        
                    //Close the socket and mark as 0 in list for reuse 
                    close( sd );  
                    client_socket[i] = 0;  
                }  
                    
                //Echo back the message that came in 
                else
                {  
                    //set the string terminating NULL byte on the end 
                    //of the data read 
                    buffer[numbytes] = '\0'; 
		    printf("%s\n",buffer); 
		    int j,sfd;
                    for (j = 0; j < max_clients; j++)
	            { 
                         sfd = client_socket[j];  
			 if(sfd == 0)
			      break;
			 else if(sfd != sd)
                              send(sfd , buffer , strlen(buffer) , 0 );  
		    }
                }  
		memset(&buffer, 0 ,sizeof buffer); 
            }  
        }  
    }  
        
    return 0;  
}  
