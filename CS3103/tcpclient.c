/* Demo TCP Client Program 
 (c) Author: Bhojan Anand
 Last Modified: 2011 Feb
 Course: CS3103/CS2105
 School of Computing, NUS
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>


int main()  //AaBee TCP Client

{

        int sock, bytes_recieved;  
        char send_data[1024],recv_data[1024];
        struct hostent *host;
        struct sockaddr_in server_addr;  

        host = gethostbyname("127.0.0.1");

		//create a Socket structure   - "Client Socket"
		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Socket");
            exit(1);
        }

        server_addr.sin_family = AF_INET;     
        server_addr.sin_port = htons(5000);   
        server_addr.sin_addr = *((struct in_addr *)host->h_addr);
        bzero(&(server_addr.sin_zero),8); 

		//connect to server at port 5000
        if (connect(sock, (struct sockaddr *)&server_addr,
                    sizeof(struct sockaddr)) == -1) 
        {
            perror("Connect");
            exit(1);
        }

         printf("\n I am conneted to (%s , %d)",
           inet_ntoa(server_addr.sin_addr),ntohs(server_addr.sin_port));  //ntoa - forms dotted decimal notation
    
        while(1)
        {
		   //Input data from user through Standard Input device
		   printf("\nTYPE ANY TEXT TO SEND (q or Q to quit) : ");
           gets(send_data);
           
          if (strcmp(send_data , "q") != 0 && strcmp(send_data , "Q") != 0)
		  {
			//send data to server
			send(sock,send_data,strlen(send_data), 0); 
			  
			//get reply from server  
			bytes_recieved=recv(sock,recv_data,1024,0);
			recv_data[bytes_recieved] = '\0';
			  
			//process data. Here, we simply print it  
			printf("\n%s ", recv_data);
          }
          else
          {//send q or Q to server and close connection 
           send(sock,send_data,strlen(send_data), 0);   
           close(sock);
           break;
          }
        
        }   
return 0;
}
