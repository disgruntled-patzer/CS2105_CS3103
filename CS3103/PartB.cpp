/*
CS3103 Assignment 1 Part B
Lau Yan Han, A0164750E
Sources:
    http://www.linuxhowtos.org/C_C++/socket.htm
    https://www.geeksforgeeks.org/socket-programming-cc/
    https://stackoverflow.com/questions/10673684/send-http-request-manually-via-socket
    Prof Bhojan Anand's sample codes
*/

#include <iostream>
#include <string.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

class tcpserver{
    
    private:

        int sock, bytes_received, port; // Socket, number of bytes received, client port
        char const *request; // HTTP request sent by client
        unsigned char reply[1024], ip_addr[20]; // Raw reply by server, and IP address extracted from reply
        struct hostent *host; // Structure containing details of host server
        struct sockaddr_in serv_addr; // Another structure containing details of host server and socket

        /*Send GET request to server to request for yourip.php*/
        void send_request(){
            send(sock, request, strlen(request), 0);
        }

        /*Receive reply from server and parse it to get the IP address*/
        void process_reply(){

            bytes_received = recv(sock, reply, 1024, 0);
            reply[bytes_received] = '\0';
            
            // Parse through reply to get IP address
            // The format of IP address in the reply is IP: a.b.c.d
            // Hence, scan through the reply until "IP:" is detected
            // Then jump forward 3 steps to reach the start of the IP address 
            for (int i = 0; reply[i] != '\0'; ++i){
                if (reply[i] == 'I' && reply[i + 1] == 'P'){
                    int start = i + 3;
                    get_ip_address(start);
                    break;
                }
            }
        }

        /*Extract the IP address from the message and prints it on the terminal*/
        void get_ip_address(int start){
            
            int j;
            while (reply[start + j] != ' '){
                ip_addr[j] = reply[start + j]; // Copy IP address into the ip_addr array
                ++j;
            }
            std::cout << "My public IP address is " << ip_addr << std::endl;

        }
    
    public:
        
        /*Constructor to initialize key socket variables*/
        tcpserver(){
            host = gethostbyname("nwtools.atwebpages.com");
            port = 80; // Default port for http requests
            request = "GET /yourip.php HTTP/1.1\r\nHost: nwtools.atwebpages.com\r\n\r\n";
        }

        /*Main function for communicating with server and parsing request*/
        bool handle_socket(){
            
            // Create socket
            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
                std::cout << "Unable to create socket\n" << std::endl;
                return false;
            }

            // Settings of socket (type, port, host address)
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(port);
            serv_addr.sin_addr = *((struct in_addr *)host->h_addr);
            bzero(&(serv_addr.sin_zero),8);

            // This is another method to get the host address, which also has a checking mechanism
            /*if (inet_pton(AF_INET, "185.176.43.88", &serv_addr.sin_addr) < 0){
                cout << "Invalid Address" << endl;
                return false;
            }*/

            // Connect to server
            int connect_result = connect(sock, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
            if (connect_result < 0){
                std::cout << "Unable to connect" << std::endl;
                return false;
            }
            
            /*std::cout << "Client has connected to " << inet_ntoa(serv_addr.sin_addr) 
                << " over port " << ntohs(serv_addr.sin_port) << std::endl;*/
            
            // Communicate with server and process reply
            send_request();
            process_reply();
            
            return true;
        } 

};

int main(int argc, char const *argv[])
{

    tcpserver CS3103;
    bool result = CS3103.handle_socket();

    return 0;
}

    // Old code to enable command line argument. No longer needed as HTTP port number must be 80
    /*if (argc < 2){
        cout << "You must specify the port number!" << endl;
        return 0;
    }

    int portno = atoi(argv[1]);

    if (portno == 0){
        cout << "Invalid Port Number " << argv[1] << " provided" << endl;
        return 0;
    }*/