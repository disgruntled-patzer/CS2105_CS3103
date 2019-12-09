/* Demo UDP Client Program 
 (c) Author: Bhojan Anand
 Last Modified: 2011 Feb
 Course: CS3103/CS2105
 School of Computing, NUS
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

int main()  //AaBee UDP Client
{
int sock, bytes_read;
unsigned int addr_len;
struct sockaddr_in server_addr;
struct hostent *host;
char send_data[1024], recv_data[1024];

//Create a datagram socket (connection less)
if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
{
perror("socket");
exit(1);
}

//Fill-up details of Server Address (IP,Port...)	
server_addr.sin_family = AF_INET; 
server_addr.sin_port = htons(5000); //Server Port
host= (struct hostent *) gethostbyname((char *)"localhost"); //server IP
server_addr.sin_addr = *((struct in_addr *)host->h_addr); //server IP
bzero(&(server_addr.sin_zero),8);

addr_len = sizeof(struct sockaddr);

   while (1)
   {
    //Input data from user through Standard Input device
    printf("\nType Something (q or Q to quit):");
    gets(send_data);
	
	//if q or Q close client    
    if ((strcmp(send_data , "q") == 0) || strcmp(send_data , "Q") == 0)
       break;

    else 
	{	
	  //Send a datagram to Server
       sendto(sock, send_data, strlen(send_data), 0,
              (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
     
		//Receive reply from Server
	   bytes_read = recvfrom(sock, recv_data, 1024, 0,
              (struct sockaddr *)&server_addr, &addr_len);
	   
	   recv_data[bytes_read] = '\0';
	   
	   //process data. Here, we just print it
	   printf("\n%s", recv_data);
	   fflush(stdout);
   }
	
   }

}
