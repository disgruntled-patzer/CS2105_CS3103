/* Demo RAW TCP PACKETS
(c) Author: Bhojan Anand
Last Modified: 2019 Sep
 Tested with the MacOS kernel version 10.14.6
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
#include<stdio.h>    //for printf
#include<string.h> //memset
#include<sys/socket.h>    //for socket ofcourse
#include<stdlib.h> //for exit(0);
#include<errno.h> //For errno - the error number
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
 

/*
    96 bit (12 bytes) pseudo header needed for tcp header checksum calculation
*/
struct pseudo_header
{
    u_int32_t source_address;
    u_int32_t dest_address;
    u_int8_t placeholder;
    u_int8_t protocol;
    u_int16_t tcp_length;
};

/*
    Generic checksum calculation function
*/
unsigned short csum(unsigned short *ptr,int nbytes)
{
    register long sum;
    unsigned short oddbyte;
    register short answer;

    sum=0;
    while(nbytes>1) {
        sum+=*ptr++;
        nbytes-=2;
    }
    if(nbytes==1) {
        oddbyte=0;
        *((u_char*)&oddbyte)=*(u_char*)ptr;
        sum+=oddbyte;
    }

    sum = (sum>>16)+(sum & 0xffff);
    sum = sum + (sum>>16);
    answer=(short)~sum;
    
    return(answer);
}

int main (void)
{
    //Create a raw socket
    int s = socket (PF_INET, SOCK_RAW, IPPROTO_TCP);
    
    if(s == -1)
    {
        //socket creation failed, may be because of non-root privileges
        perror("Failed to create socket");
        exit(1);
    }
    
    //Datagram to represent the packet
    char datagram[4096] , source_ip[32] , *data , *pseudogram;
    
    //zero out the packet buffer
    memset (datagram, 0, 4096);
    
    //IP header
    struct ip *iph = (struct ip *) datagram;
    
    //TCP header
    struct tcphdr *tcph = (struct tcphdr *) (datagram + sizeof (struct ip));
    struct sockaddr_in sin;
    struct pseudo_header psh;
    
    //Data part
    data = datagram + sizeof(struct ip) + sizeof(struct tcphdr);
    strcpy(data , "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    
    //some address resolution
    strcpy(source_ip , "192.168.0.101"); //your system's IP address
    sin.sin_family = AF_INET;
    //sin.sin_port = htons(80);
    sin.sin_addr.s_addr = inet_addr ("137.132.10.11"); //luminus.nus.edu.sg
  

    //Fill in the IP Header
    
    iph->ip_hl = 5;
    iph->ip_v = 4;
    iph->ip_tos = 0;
    iph->ip_len = sizeof (struct ip) + sizeof (struct tcphdr) + strlen(data);
    iph->ip_id = htons (54321);    //Id of this packet
    iph->ip_off = 0;
    iph->ip_ttl = 1;
    iph->ip_p = IPPROTO_TCP;
    iph->ip_sum = 0;        //Set to 0 before calculating checksum
    iph->ip_src.s_addr = inet_addr ( source_ip );    //Spoof the source ip address
    iph->ip_dst.s_addr = sin.sin_addr.s_addr;
   
    //Ip checksum
    iph->ip_sum = csum ((unsigned short *) datagram, iph->ip_len);
    
    //TCP Header
    tcph->th_sport = htons (1234);
    tcph->th_dport = htons (8080);
    tcph->th_seq = htonl(0);
    tcph->th_ack = 0;  //first SYN packet will not have ACK
    tcph->th_off = 5;    //tcp header size
    // tcph->th_off = sizeof(struct tcphdr)/4; /* data position in the packet */
    tcph->th_flags = TH_SYN; /* initial connection request */
    //tcph->th_flags = TH_PUSH;
    tcph->th_win = htons (5840);    /* maximum allowed window size */
    tcph->th_sum = 0;    //leave checksum 0 now, filled later by pseudo header
    tcph->th_urp = 0;
    
   

    
    //Now the TCP checksum
    psh.source_address = inet_addr( source_ip );
    psh.dest_address = sin.sin_addr.s_addr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_TCP;
    psh.tcp_length = htons(sizeof(struct tcphdr) + strlen(data) );
    
    int psize = sizeof(struct pseudo_header) + sizeof(struct tcphdr) + strlen(data);
    pseudogram = (char*) malloc(psize);
    
    memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
    memcpy(pseudogram + sizeof(struct pseudo_header) , tcph , sizeof(struct tcphdr) + strlen(data));
    
    tcph->th_sum = csum( (unsigned short*) pseudogram , psize);
    
    //IP_HDRINCL to tell the kernel that headers are included in the packet
    int one = 1;
    const int *val = &one;
    
    if (setsockopt (s, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0)
    {
        perror("Error setting IP_HDRINCL");
        exit(0);
    }
    
    //loop if you want to flood :)
    int loop =2;
    while (loop<15)
    {
  
        //Send the packet
        if (sendto (s, datagram, iph->ip_len ,    0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
        {
            perror("sendto failed");
        }
        //Data send successfully
        else
        {
            printf ("Packet Send. Length : %d \n" , iph->ip_len);
        }
        loop++;
    }
	
	return 0;
}

//Complete
