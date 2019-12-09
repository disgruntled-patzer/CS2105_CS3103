/* Demo UDP ECHO Server Program 
 (c) Author: Bhojan Anand
 Last Modified: 2011 Feb
 Course: CS3103/CS2105
 School of Computing, NUS
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

int main()   //AaBee UDP Server
{
        int sock;
        int  bytes_read;
        unsigned int addr_len;
        char recv_data[1024], reply_data[1024];
        struct sockaddr_in server_addr , client_addr;

		//Create a datagram socket (connection less)
        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
            perror("Socket");
            exit(1);
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(5000);
        server_addr.sin_addr.s_addr = INADDR_ANY;
        bzero(&(server_addr.sin_zero),8);

		//bind the sockt to local Internet Address (IP and PORT).  
        if (bind(sock,(struct sockaddr *)&server_addr,
            sizeof(struct sockaddr)) == -1)
        {
            perror("Bind");
            exit(1);
        }

        addr_len = sizeof(struct sockaddr);
		
	printf("\n UDPServer Waiting for DATA from client on port 5000");
        fflush(stdout);

	while (1)
	{
	  //receive data from the client
	  bytes_read = recvfrom(sock,recv_data,1024,0,
					(struct sockaddr *)&client_addr, &addr_len);
	  recv_data[bytes_read] = '\0';
	  
	 //process data. Here, we just print it and Reply to Client
	 printf("\n(%s , %d) said : ",inet_ntoa(client_addr.sin_addr),
                                       ntohs(client_addr.sin_port));
	 printf("%s", recv_data);
	 fflush(stdout);
	
		
	 strcpy(reply_data, "");
	 strcat(reply_data, "ECHO FROM SERVER: ");
	 strcat(reply_data, recv_data);
	 
		
	 sendto(sock,reply_data,strlen(reply_data),0,
							  (struct sockaddr *)&client_addr, addr_len);	
		
	}
 return 0;
}
