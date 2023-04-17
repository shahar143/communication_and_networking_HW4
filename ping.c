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


// IPv4 header len without options
#define IP4_HDRLEN 20

// ICMP header len for echo req
#define ICMP_HDRLEN 8

#define PACKET_LEN 56

// Checksum algo
unsigned short calculate_checksum(unsigned short *paddress, int len);
bool isValidIpAddress(char *ipAddress);


#define SOURCE_IP "10.0.2.15"
// i.e the gateway or ping to google.com for their ip-address

int main(int count, char *argv[])

{
    // To check if we get two arguments in the console.
    if ( count != 2 )
    {
        printf("usage: ./ping <addr> \n");
        exit(-1);
    }

    else { // To check if the ip is correct.
        if (!isValidIpAddress(argv[1])) {
            printf("ip address is not Valid\n");
            exit(-1);
        } else {


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

            while (1) { // This loop will help us , to send infinite ping.


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


                // Next, ICMP header
                memcpy((packet), &icmphdr, ICMP_HDRLEN);

                // After ICMP header, add the ICMP data.
                memcpy(packet + ICMP_HDRLEN, data, datalen);


                ((struct icmp *) packet)->icmp_cksum = calculate_checksum((unsigned short *) packet,
                                                                          ICMP_HDRLEN + datalen);


                gettimeofday(&start, 0);

                // Send the packet using sendto() for sending datagrams.

                if (flag == 0) {
                    printf("PING %s (%s): %d bytes of data\n", inet_ntoa(dest_in.sin_addr),
                           inet_ntoa(dest_in.sin_addr), PACKET_LEN);
                    flag = 1;
                }

                int bytes_sent = sendto(sock, packet, ICMP_HDRLEN + datalen, 0, (struct sockaddr *) &dest_in,
                                        sizeof(dest_in));
                if (bytes_sent == -1) {
                    fprintf(stderr, "sendto() failed with error: %d", errno);
                    return -1;
                }


                // Get the ping response
                bzero(&packet, IP_MAXPACKET);
                socklen_t len = sizeof(dest_in);
                ssize_t bytes_received = -1;


                bytes_received = recvfrom(sock, packet, sizeof(packet), 0, (struct sockaddr *) &dest_in, &len);


                if (bytes_received > 0) { // if we get response then
                    gettimeofday(&end, 0);


                    float milliseconds =
                            (end.tv_sec - start.tv_sec) * 1000.0f + (end.tv_usec - start.tv_usec) / 1000.0f;


                    //Check the IP header
                    struct iphdr *iphdr = (struct iphdr *) packet;

                    printf("%ld bytes from %s: seq=%d, ttl=%hhu time=%.2f ms\n", datalen + bytes_received,
                           inet_ntoa(dest_in.sin_addr), ++icmphdr.icmp_seq, iphdr->ttl, milliseconds);


                    char reply[IP_MAXPACKET];
                    memcpy(reply, packet + ICMP_HDRLEN + IP4_HDRLEN, datalen);
                }

                sleep(1);

            }
        }
    }
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