/*
 CAPTURE ICMP MESSAGES USING RAW SOCKET
 (c) Author: Bhojan Anand
 Last Modified: 2017 Sep
 Course: CS3103
 School of Computing, NUS
 */


# include <unistd.h>
# include <sys/socket.h>
# include <sys/types.h>
#include  <arpa/inet.h>
# include <string.h>
# include <stdio.h>
# include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>


int main(){    //AaBee ICMP message capture

    
    int sockfd;
    long n;
    socklen_t clilen;
    struct sockaddr_in cliaddr;
    char buf[10000];
    int i;
    
    //Create a RAW socket (connection less)
    sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0){
        perror("sock:");
        exit(1);
    }
    
    
    clilen = sizeof(struct sockaddr_in);
    while(1){
        //receive data from the client and store in 'buf'
        printf("\nWAITING FOR ICMP DATA:\n");
        n=recvfrom(sockfd,buf,10000,0,(struct sockaddr *)&cliaddr,&clilen);
        //process data.PRINT the data
        printf(" received %ld bytes\n", n);
        
        //extract IP header from the buffer 'buf'
        struct ip *ip_hdr = (struct ip *)buf;
        
        printf("IP header is %d bytes.\n", ip_hdr->ip_hl*4);
        
        //print all the values in the packet in Hex
        for (i = 0; i < n; i++) {
            printf("%02X%s", (uint8_t)buf[i], (i + 1)%16 ? " " : "\n");
        }
        printf("\n");
        //extract ICMP  header part
        struct icmp *icmp_hdr = (struct icmp *)((char *)ip_hdr + (4 * ip_hdr->ip_hl));
        
        printf("ICMP msgtype=%d, code=%d", icmp_hdr->icmp_type, icmp_hdr->icmp_code );
        
        // Parse/decode the Hex output of packet data using ... www.rodneybeede.com/IP__IPv4_or_IPv6__packet_hex_dump_parser.html
        
    }
    return 0;
}
