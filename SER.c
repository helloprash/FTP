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
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>


#define PORT "3491"  // the port users will be connecting to
#define MAXDATASIZE 100 // max number of bytes we can get at once
#define BACKLOG 10     // how many pending connections queue will hold

//-----GPIO Constants
#define BCM2708_PERI_BASE       0x3F000000
#define GPIO_BASE               (BCM2708_PERI_BASE + 0x200000)  // GPIO controller
#define BLOCK_SIZE              (4*1024)

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
//--------GPIO Config-----------------
// IO Acces
struct bcm2835_peripheral {
    unsigned long addr_p;
    int mem_fd;
    void *map;
    volatile unsigned int *addr;
};

struct bcm2835_peripheral gpio = {GPIO_BASE};

void IN_GPIO(int g)
{
   /* int bitMask;
    volatile unsigned int *GPSEL ;
    bitMask = 0x7 ;   //0000 0000 0000 0000 0000 0000 0000 0111
    bitMask = bitMask << ((g)%10)*3 ;

    // NOW WE WIll do bit wise compliment of all bits in the bitMask
    bitMask = ~bitMask ;

    //*gpio.addr  has the base address

    GPSEL = (gpio.addr + (g/10)) ;  // Add base address to pin/10 to get the GPSEL register address
    //GPSEL is pointing to the memory which has the configuraiton information of the pin
    *GPSEL = *GPSEL & bitMask ;*/
   *(gpio.addr + ((g)/10)) &= ~(7<<(((g)%10)*3));
}

void OUT_GPIO(int g)
{
   /* int bitMask;
    volatile unsigned int *GPSEL ;
    bitMask = 0x1 ;   //0000 0000 0000 0000 0000 0000 0000 0001
    bitMask = bitMask << ((g)%10)*3 ;
    GPSEL=gpio.addr+g/10;/*for pins 0 to 9 this will be same*/
   /**GPSEL = *GPSEL | bitMask ;*/
     IN_GPIO(g);
    *(gpio.addr + ((g)/10)) |=  (1<<(((g)%10)*3));
}

void SetGPIO(int g)
{
   *(gpio.addr + 7) = 1 << g ;
}

void ClearGPIO(int g)
{
  *(gpio.addr + 10)  = 1 << g ;

}

// Exposes the physical address defined in the passed structure using mmap on /dev/mem
int map_peripheral(struct bcm2835_peripheral *p)
{
   // Open /dev/mem
   if ((p->mem_fd = open("/dev/mem", O_RDWR) ) < 0)
   {
      printf("Failed to open /dev/mem, try checking permissions.\n");
      return -1;
   }

   p->map = mmap(
      NULL,
      BLOCK_SIZE,
      PROT_READ|PROT_WRITE,
      MAP_SHARED,
      p->mem_fd,      // File descriptor to physical memory virtual file '/dev/mem'
      p->addr_p       // Address in physical map that we want this memory block to expose
   );

   if (p->map == MAP_FAILED)
   {
        perror("mmap");
        return -1;
   }
   p->addr = (volatile unsigned int *)p->map;

   return 0;
}

int main(void)
{
    int sockfd, clientToken ;  // listen on sock_fd, new connection on clientToken
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    int retVal ;
    int numbytes ;
    FILE *ptr;
    char filename[50];
    char message[30];
    char rasp[50];
    int ledStatus[26];
    int ledNo;

    if(map_peripheral(&gpio) == -1)
    {
        printf("Failed to map the physical GPIO registers into the virtual memory space.\n");
        return -1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1)
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

  //freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    retVal = listen(sockfd, BACKLOG);
    if (retVal == -1)
    {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    retVal = sigaction(SIGCHLD, &sa, NULL);

    if (retVal == -1)
    {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1)
    {  // main accept() loop
        sin_size = sizeof their_addr;
        clientToken = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (clientToken == -1)
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
        {
            // this is the child process
            close(sockfd); // child doesn't need the listener
            strcpy(message, "Welcome to LED control");
            retVal = send(clientToken, message, strlen(message) , 0);//Welcome
            if (retVal == -1)
            {
                perror("send");
            }
             
			strcpy(filename,"led.html");
            ptr = fopen(filename,"w");
            if(ptr == NULL)
            {
                printf("Unbale to open html file\n");
                perror("FILE");
            }

			while(1)
            {
               memset(&rasp[0], 0, sizeof(rasp));
               memset(&message[0], 0, sizeof(message));

               numbytes = recv(clientToken, rasp, sizeof(rasp), 0); //Option and ledNo
               if (numbytes <= 0)
               {
                   printf("Receive failed \n");
               }

                        printf("Outside html\n");
                        fprintf(ptr,"<html>\n");
                        printf("Inside html %x \n",*ptr);
                        fprintf(ptr,"<head><style>table, th, td { border: 1px solid black; border-collapse: collapse; }</style></head>\n");
                        fprintf(ptr,"<body>\n");
                        fprintf(ptr,"<table>\n");
                        fprintf(ptr,"<tr><th>LED Number</th><th>Status</th></tr>\n");

                sscanf(rasp,"%s %d",message, &ledNo);
                
                IN_GPIO(ledNo);
                OUT_GPIO(ledNo);

                if(strcasecmp(message,"ON") == 0)
                {
                    if(ledStatus[ledNo-2] == 1)
                    {
                        fprintf(ptr,"<tr><td>%d</td><td>Already ON</td></tr>\n",ledNo);
                    }
                    else
                    {
                    	printf("LED ON\n");
                        SetGPIO(ledNo);
                        ledStatus[ledNo-2] = 1;
                        fprintf(ptr,"<tr><td>%d</td><td>ON</td></tr>\n",ledNo);
                    }
                }
 			   else if(strcasecmp(message,"OFF") == 0)
               {
                    if(ledStatus[ledNo-2] == 0)
                    {
                        fprintf(ptr,"<tr><td>%d</td><td>Already OFF</td></tr>\n",ledNo);
                    }
                    else
                    {
                         printf("LED OFF\n");
                         ClearGPIO(ledNo);
                         ledStatus[ledNo-2] = 0;
                         fprintf(ptr,"<tr><td>%d</td><td>OFF</td></tr>\n",ledNo);
                    }
                }
                else if(strcasecmp(message,"STATUS") == 0)
                {
                     if(ledStatus[ledNo-2] == 1)
                     {
                        fprintf(ptr,"<tr><td>%d</td><td>ON</td></tr>\n",ledNo);
                     }
                     else if(ledStatus[ledNo-2] == 0)
                     {
                        fprintf(ptr,"<tr><td>%d</td><td>OFF</td></tr>\n",ledNo);
                     }
                }
                else if (strcasecmp(rasp,"Quit") == 0)
                {
                    printf("Child:  is Quitting %s\n" ,rasp);
                      break ;
                }
                else
                {
                    printf("Child else:  |%s|\n",rasp );
                }

                fprintf(ptr,"</table>\n");
                fprintf(ptr,"</body>\n");
                fprintf(ptr,"</html>\n");
            }
            
			printf("before closing %x\n", ptr);
            close(clientToken);
            printf("after closing %x\n", ptr);
            fclose(ptr);
            exit(0);
        }
        else if (retVal < 0)
        {
             printf("fork failed \n");
        }
        // The following is in parent code this happens only whe retVal > 0.
        close(clientToken);  // parent doesn't need this
   }
    return 0;
}


