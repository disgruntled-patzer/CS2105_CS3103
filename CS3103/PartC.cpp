/*
CS3103 Assignment 1 Part C
Lau Yan Han, A0164750E
Sources:
    https://www.binarytides.com/raw-sockets-c-code-linux/
    https://opensourceforu.com/2015/03/a-guide-to-using-raw-sockets/
    http://squidarth.com/networking/systems/rc/2018/05/28/using-raw-sockets.html
    http://www.cs.binghamton.edu/~steflik/cs455/rawip.txt
    https://www.binarytides.com/tcp-syn-portscan-in-c-with-linux-sockets/
    https://www.binarytides.com/raw-sockets-c-code-linux
    Prof Bhojan Anand's sample codes

Assumptions:
    Source IP is the private IP address of the PC running this program, on wlp2s0 interface
*/

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <fcntl.h>

class PartC{
    private:

        /*This is a pseudo header that is required to calulate tcp checksum*/
        struct tcp_chksum_header{
            u_int32_t source_addr;
            u_int32_t dest_addr;
            u_int8_t placeholder;
            u_int8_t protocol;
            u_int16_t tcp_len;
        };

        // Two sockets will be created:
        // (1) A ping socket to send and receive TCP messages of varying TTL, and
        // (2) an icmp socket to listen for incoming icmp messages.
        int ping_sock, icmp_sock; // The two socketes used in this program
        struct ip *ip_hdr, *recv_ip_hdr; // IP headers; ip_hdr is for outgoing TCP msgs, recv_ip_hdr is for incoming icmp msgs
        struct tcphdr *tcp_hdr, *recv_tcp_hdr; // TCP headers; same nomenclature as IP headers
        struct icmp *recv_icmp_hdr; // ICMP headers. Only incoming ICMP messages are required
        
        // Information on the host url, used by ping socket
        struct hostent *host;
        socklen_t host_len;
        struct sockaddr_in host_addr;
        long recv_bytes;
        
        char send_msg[1024], recv_msg[1024], source_ip[32], dest_ip[32], *send_msg_ptr, *tcp_chksum_buffer;

        // Pseudo header used to calculate tcp checksum
        struct tcp_chksum_header tcp_chk;

        /*Calculate checksum for ip header. Taken from Prof Bhojan Anand's example code*/
        unsigned short calc_chksum(unsigned short *ptr,int nbytes){
            
            register long sum;
            unsigned short oddbyte;
            register short answer;

            sum = 0;
            while(nbytes > 1) {
                sum += *ptr ++;
                nbytes -= 2;
            }
            if(nbytes == 1) {
                oddbyte = 0;
                *((u_char*)&oddbyte) = *(u_char*)ptr;
                sum += oddbyte;
            }

            sum = (sum>>16)+(sum & 0xffff);
            sum = sum + (sum>>16);
            answer = (short)~sum;
    
            return answer;
        }

    public:
        
        /*Create ping and icmp sockets*/
        void create_sockets(const char *url, int ping_port, const char *src_ip){
            
            // Create ping socket to send TCP SYNC messags to host
            if ((ping_sock = socket(PF_INET, SOCK_RAW, IPPROTO_TCP)) < 0){
                perror("Failed to create ping socket. You should run the program with sudo");
                exit(1);
            }

            // Initialize ping socket variables
            if ((host = gethostbyname(url)) == NULL){
                perror("Error in resolving URL");
                exit(1);
            }
            host_addr.sin_family = AF_INET;
            host_addr.sin_port = htons(ping_port);
            host_addr.sin_addr = *((struct in_addr *)host->h_addr);
            bzero(&(host_addr.sin_zero), 8);
            strcpy(dest_ip, inet_ntoa(host_addr.sin_addr)); // Save the final destination IP for later use
            memset(send_msg, 0, 1024); // zero out the send_msg buffer
            printf("Pinging %s (%s) (max 30 hops) \n", url, dest_ip);

            // Initialize ip and tcp headers, and send_msg data for ping socket.
            // We point ip header structure to the send_msg, point a
            // tcp header structure after that, and the data payload after that
            ip_hdr = (struct ip *)send_msg;
            strcpy(source_ip, src_ip);
            tcp_hdr = (struct tcphdr *)(send_msg + sizeof(struct ip));
            send_msg_ptr = send_msg + sizeof(struct ip) + sizeof(struct tcphdr);
            strcpy(send_msg_ptr, "TCP_SYN");

            // Fill in the ping socket IP header
            ip_hdr->ip_hl = 5; // IP header length
            ip_hdr->ip_v = 4; // IP version (IPv4)
            ip_hdr->ip_tos = 0; // Type of service
            ip_hdr->ip_len = sizeof(struct ip) + sizeof(struct tcphdr) + strlen(send_msg_ptr); // Length of IP packet
            ip_hdr->ip_id = htons(54321); // ID number (we can use any ID no. we want)
            ip_hdr->ip_off = 0; // IP offset
            ip_hdr->ip_p = IPPROTO_TCP; // Protocol
            ip_hdr->ip_sum = 0; // Fill in the checksum later
            ip_hdr->ip_src.s_addr = inet_addr(source_ip); // Source IP address
            ip_hdr->ip_dst.s_addr = host_addr.sin_addr.s_addr; // Destination IP address

            // Calculate ping socket IP Checksum
            ip_hdr->ip_sum = calc_chksum((unsigned short *)send_msg, ip_hdr->ip_len);

            // Fill in the ping socket tcp header
            tcp_hdr->th_sport = htons(14550); // Source port
            tcp_hdr->th_dport = htons(ping_port); // Destination port
            tcp_hdr->th_seq = htonl(0); // First pkt starts at 0 seq
            tcp_hdr->th_ack = 0; // First SYN pkt has no ACK
            tcp_hdr->th_off = 5; // Size of tcisp header determines the offset
            tcp_hdr->th_flags = TH_SYN; // Initial connection request
            tcp_hdr->th_win = htons(5840); // Max allowed window size
            tcp_hdr->th_sum = 0; // Fill in the TCP checksum later
            tcp_hdr->th_urp = 0; // No urgent pointers
            // IP ttl will be set later in run_sockets

            // Calculate ping socket TCP checksum. This is taken from Prof Bhojan Anand's example
            // The basic idea is to copy the required info into tcp_chksum_buffer (the array containing pseudo header info)
            // and then calculate the checksum from there
            tcp_chk.source_addr = inet_addr(source_ip);
            tcp_chk.dest_addr = host_addr.sin_addr.s_addr;
            tcp_chk.placeholder = 0;
            tcp_chk.protocol = IPPROTO_TCP;
            tcp_chk.tcp_len = htons(sizeof(struct tcphdr) + strlen(send_msg_ptr));
            int tcp_chksize = sizeof(struct tcp_chksum_header) + sizeof(struct tcphdr) + strlen(send_msg_ptr);
            tcp_chksum_buffer = (char*) malloc(tcp_chksize);
            memcpy(tcp_chksum_buffer, (char*)&tcp_chk, sizeof(struct tcp_chksum_header));
            memcpy(tcp_chksum_buffer + sizeof(struct tcp_chksum_header), tcp_hdr, sizeof(struct tcphdr) + strlen(send_msg_ptr));
            tcp_hdr->th_sum = calc_chksum( (unsigned short*) tcp_chksum_buffer, tcp_chksize);

            // Perform IP_HDRINCL (IP headers are included in send_msg). This prevents kernel from adding its own IP/TCP headers
            int one = 1;
            const int *val = &one;
            if ((setsockopt(ping_sock, IPPROTO_IP, IP_HDRINCL, val, sizeof(one))) < 0){
                perror("Error setting IP_HDRINCL");
                exit(1);
            }

            // Create icmp socket to receive icmp messages
            if ((icmp_sock = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0){
                perror("Failed to create icmp socket");
                exit(1);
            }

            // Set icmp socket to be non-blocking. This prevents the program from hanging
            // when any intermediate routers do not send icmp messages, or the destination router sends a TCP reply
            int flags;
            if ((flags = fcntl(icmp_sock, F_GETFL)) < 0){
                perror("Error getting flags on icmp socket");
                exit(1);
            }
            if ((fcntl(icmp_sock, F_SETFL, flags | O_NONBLOCK)) < 0){
                perror("Error setting icmp socket to non-blocking");
                exit(1);
            }

            // Deprecated
            // Set timeout: https://stackoverflow.com/questions/4181784/how-to-set-socket-timeout-in-c-when-making-multiple-connections
            /*struct timeval tv;
            tv.tv_sec = 5;
            setsockopt(icmp_sock, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(struct timeval));*/
        }

        /*Run traceroute on ping and icmp sockets*/
        void run_sockets(){

            // These will be used to detect if any intermediate routers are not sending icmp messages
            char prev_recv_ip[32] = "", curr_recv_ip[32];

            // TTL is initialized here
            ip_hdr->ip_ttl = 1;
            
            while (true){

                // Ping socket sends TCP_SYNC messages, after which TTL is increased for the next loop
                sendto (ping_sock, send_msg, ip_hdr->ip_len, 0, (struct sockaddr *)&host_addr, sizeof(host_addr));
                ip_hdr->ip_ttl++;

                // Listen for incoming icmp messages, and extract the incoming ip and icmp headers
                host_len = sizeof(struct sockaddr_in);
                recv_bytes = recvfrom(icmp_sock, recv_msg, 1024, 0, (struct sockaddr *)&host_addr, &host_len);
                recv_ip_hdr = (struct ip *)recv_msg;
                recv_icmp_hdr = (struct icmp *)((char *)recv_ip_hdr + (4 * recv_ip_hdr->ip_hl));
                
                // Update the current IP address received by the icmp socket
                strcpy(curr_recv_ip, inet_ntoa(recv_ip_hdr->ip_src));
                
                // If an intermediate router does not send any icmp message, the current IP address
                // received by the icmp socket will not be updated, and is thus the same as the previous
                // icmp message. Use this to detect that a router isn't responding, and highlight it with * * *
                // Otherwise, if the incoming message is from the host url, we have reached the destination
                // Otherwise, check whether the incoming icmp error message is due to TTL exceeding; if so, print
                // the ip of the intermediate router
                if (strcmp(curr_recv_ip, prev_recv_ip) == 0){
                    printf("(ttl = %d): * * *\n", ip_hdr->ip_ttl);
                }
                else if (strcmp(curr_recv_ip, dest_ip) == 0){
                    printf("Final destination reached: %s\n", dest_ip);
                    break;
                }
                else if (recv_icmp_hdr->icmp_type == 11){
                    printf("Intermediate router (ttl = %d): %s\n", ip_hdr->ip_ttl, curr_recv_ip);
                }


                // Set a limit of 30 hops like the traditional traceroute tool
                if (ip_hdr->ip_ttl == 30){
                    printf("Traceroute limit reached. Final destination = %s\n", dest_ip);
                    break;
                }

                // Save the last received IP address for comparison on the next loop
                strcpy(prev_recv_ip, curr_recv_ip);
                sleep(1);
            }
            
            close(ping_sock);
            close(icmp_sock);

        }
};

int main(int argc, char const *argv[])
{
    if (argc < 3){
        perror("Usage: sudo <program name> <source_ip address> <target URL>");
        exit(1);
    }
    else{
        //printf("Pinging %s (max 30 hops) \n", argv[2]);
        int ping_port = 14551; //non-existent port on host
        PartC CS3103;
        CS3103.create_sockets(argv[2], ping_port, argv[1]);
        CS3103.run_sockets();
    }
    return 0;
}