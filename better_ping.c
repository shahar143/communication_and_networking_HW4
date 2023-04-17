//
// Created by codebind on 12/24/22.
//

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h> // gettimeofday()
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <strings.h>
#include <netinet/tcp.h>
#include <sys/wait.h>
#include <fcntl.h> /* Added for the nonblocking socket */

// IPv4 header len without options
#define IP4_HDRLEN 20

// ICMP header len for echo req
#define ICMP_HDRLEN 8

#define PACKET_LEN 56

// Checksum algo
unsigned short calculate_checksum(unsigned short *paddress, int len);
bool isValidIpAddress(char *ipAddress);



#define WATCH_DOG_PORT 3000



int main(int count, char *argv[]) {

    char *args[2];
    // compiled watchdog.c by makefile
    args[0] = "./watchdog";
    args[1] = argv[1];
    int status;
    int pid = fork();
    if (pid == 0)
    {
        // Executing the watchdog program.
        execvp(args[0], args);

        fprintf(stderr,"Error starting watchdog\n");
        perror("execvp");
        exit(errno);

    }
    else {

        int counter_s =0 ;
        int counter_r =0 ;
        int counter_l =0 ;
        float total_time =0;


        // create the socket file descriptor -> an integer that will represent the connection
        int client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        // if the socket file descriptor equals to -1 then the socket wasn't created
        if (client_socket_fd == -1) {
            printf("Could not create socket: %d", errno);
        }

        // create the server_address struct that contains the ip protocol and the server port
        struct sockaddr_in server_address;
        // zero all bites of the server_address spot in memory
        memset(&server_address, 0, sizeof(server_address));

        //saving the ipv4 type and the server port using htons to convert the server port to network's order
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
        server_address.sin_port = htons(WATCH_DOG_PORT);

        if (setsockopt(client_socket_fd, SOL_SOCKET, SO_REUSEADDR, "cubic", 5) == -1) {
            perror("setsockopt() has failed");
            exit(-1);
        }


        if (bind(client_socket_fd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
            perror("bind has failed");
            close(client_socket_fd);
            exit(-1);
        }


        if (listen(client_socket_fd, 10) < 0) {
            perror("listen has failed");
            close(client_socket_fd);
            exit(-1);
        }

        // prepare the internet socket address.
        struct sockaddr_in client_address;
        bzero(&client_address, sizeof(client_address));
        socklen_t client_address_len = sizeof(client_address);

        // Waiting to a connection , server will accept requests from client with accept method
        int client_socket = accept(client_socket_fd, (struct sockaddr *) &server_address, &client_address_len);
        if (client_socket == -1) {
            perror("accept has failed\n");
            close(client_socket);
            exit(-1);
        }

        usleep(1000);

///////////////////////////////////////////////////////////////////////////////////////////////////////

        if (count != 2) {
            printf("usage: ./better_ping <addr> \n");
            exit(-1);
        }

        //After converting the address to a binary/char then check if the address is correct or not.
        if (!isValidIpAddress(argv[1])) {
            printf("ip address is not Valid\n");
            exit(-1);
        }

        char data[IP_MAXPACKET] = "This is the ping.\n";
        int flag = 0;

        int datalen = strlen(data);
        struct icmp icmphdr;
        // Sequence Number (16 bits): starts at 0
        icmphdr.icmp_seq = 0;


        struct sockaddr_in dest_in;
        memset(&dest_in, 0, sizeof(struct sockaddr_in));
        dest_in.sin_family = AF_INET;
        dest_in.sin_addr.s_addr = inet_addr(argv[1]);


        // Create raw socket for IP-RAW (make IP-header by yourself)
        int sock = -1;
        if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
            fprintf(stderr, "socket() failed with error: %d", errno);
            fprintf(stderr, "To create a raw socket, the process needs to be run by Admin/root user.\n\n");
            return -1;
        }

        struct timeval start, end;
        char packet[IP_MAXPACKET];
////////////////////////////////////////////////////////////////////////////////////////////
        float milliseconds =0;
        ssize_t bytes_received = -1;

        while (1) { // Build infinite loop , to send an infinite ping.

            icmphdr; // ICMP-header
            //===================
            // ICMP header
            //===================

            // Message Type (8 bits): ICMP_ECHO_REQUEST
            icmphdr.icmp_type = 8;

            // Message Code (8 bits): echo request
            icmphdr.icmp_code = 0;

            // Identifier (16 bits): some number to trace the response.
            // It will be copied to the response packet and used to map response to the request sent earlier.
            // Thus, it serves as a Transaction-ID when we need to make "ping"
            icmphdr.icmp_id = 18;

            // ICMP header checksum (16 bits): set to 0 not to include into checksum calculation
            icmphdr.icmp_cksum = 0;

            // Combine the packet
            memcpy((packet), &icmphdr, ICMP_HDRLEN);

            // After ICMP header, add the ICMP data.
            memcpy(packet + ICMP_HDRLEN, data, datalen);


            ((struct icmp *) packet)->icmp_cksum = calculate_checksum((unsigned short *) packet,
                                                                      ICMP_HDRLEN + datalen);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            gettimeofday(&start, 0);

            if (flag == 0) { // Only to print th  first line of the ping.
                printf("PING %s (%s): %d bytes of data\n", inet_ntoa(dest_in.sin_addr),
                       inet_ntoa(dest_in.sin_addr), PACKET_LEN);
                flag = 1;
            }

            /*Send N bytes of BUF on socket FD to peer at address ADDR (which is
                ADDR_LEN bytes long).  Returns the number sent, or -1 for errors.
                    */
            //Send a echo packet (request).
            int bytes_sent = sendto(sock, packet, ICMP_HDRLEN + datalen, 0, (struct sockaddr *) &dest_in,
                                    sizeof(dest_in));
            if (bytes_sent == -1) {
                fprintf(stderr, "sendto() failed with error: %d", errno);
                return -1;
            }
            else{
                counter_s++;
            }


            usleep(1000);
            total_time+=1000;
            /* Change the socket into non-blocking state	*/
            fcntl(sock, F_SETFL, O_NONBLOCK);



            // Get the ping response
            bzero(&packet, IP_MAXPACKET);
            socklen_t len = sizeof(dest_in);


            // Get response from (echo reply).
            bytes_received = recvfrom(sock, packet, sizeof(packet), 0, (struct sockaddr *) &dest_in, &len);

            usleep(1000);
            total_time+=1000;

            if (bytes_received==-1){
                /* check if there is no response then send a single to the watchdog using a tcp connection.
                 then well the timer is begun , and if we get the response then th timer well upload to zero ,
                 if not and get the 10 then well print a message exit and close the connection, exit the program. */

                char msg[1];

                bzero(msg,0);
                recv(client_socket,&msg, sizeof(msg),0);


                if (strcmp(msg,"m")==0){

                    continue;

                }
                //counter_l++;
            }


            gettimeofday(&end, 0);


            if (bytes_received > 0) {

                // if we get a response then send a  msg to the watchdog , to upload the timer to zero.
                char bufferek[1] = "k";
                int f_send = send(client_socket, bufferek, sizeof(bufferek), 0);
                if (f_send == -1) {
                    printf("k message has failed to send\n");
                    close(client_socket);
                    exit(-1);
                }



                //Check the IP header
                struct iphdr *iphdr = (struct iphdr *) packet;
                milliseconds =
                        (float) (end.tv_sec - start.tv_sec) * 1000.0f + (float)(end.tv_usec - start.tv_usec) / 1000.0f;
                printf("%ld bytes from %s: icmp_seq=%d, ttl=%hhu time=%.2f ms\n", datalen + bytes_received,
                       inet_ntoa(dest_in.sin_addr), ++icmphdr.icmp_seq, iphdr->ttl, milliseconds);

                total_time+=milliseconds;



                char reply[IP_MAXPACKET];
                memcpy(reply, packet + ICMP_HDRLEN + IP4_HDRLEN, datalen);


                counter_r++;


            }
            else { // recv a signl to watchdog if their no recv , to check the timer.

                counter_l++;

                char msgr[1];
                recv(client_socket,&msgr, sizeof(msgr),0);

                if (strcmp(msgr,"n")==0){
                    printf("server <%s> cannot be reached\n", argv[1]);

                    close(client_socket_fd);
                    close(client_socket);
                    printf("\n--- %s ping statistics --- \n",argv[1]);
                    printf("%d packets transmitted, %d received, %d%% packet loss, time %fms\n\n",counter_s,counter_r,((counter_l + 1)* 10),total_time);
                    exit(-1);

                }

            }

            sleep(1); // to send a ping for each one second.
            total_time+= 10000;
        }
    }



    // Close the raw socket descriptor.
    //close(sock);


    return 0;
}

// Compute checksum (RFC 1071).
unsigned short calculate_checksum(unsigned short *paddress, int len)
{
    int nleft = len;
    int sum = 0;
    unsigned short *w = paddress;
    unsigned short answer = 0;

    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1)
    {
        *((unsigned char *)&answer) = *((unsigned char *)w);
        sum += answer;
    }

    // add back carry outs from top 16 bits to low 16 bits
    sum = (sum >> 16) + (sum & 0xffff); // add hi 16 to low 16
    sum += (sum >> 16);                 // add carry
    answer = ~sum;                      // truncate to 16 bits

    return answer;
}

bool isValidIpAddress(char *ipAddress)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
    return result != 0;
}