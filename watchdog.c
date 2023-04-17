
#include "share_header.h"
////#define SOURCE_IP "127.0.0.1"
///// ping to google.com
////#define DESTINATION_IP "8.8.8.8"
#define PORT 3000



int main(int count,char *argv[]) {
    int socket_fd;
    struct sockaddr_in watchdog_addr;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        printf("socket creation failed\n");
        close(socket_fd);
        exit(-1);
    }

    bzero(&watchdog_addr, sizeof(watchdog_addr));


    //assign IP , PORT to watchdog
    watchdog_addr.sin_family = AF_INET;
    watchdog_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    watchdog_addr.sin_port = htons(PORT);



    if (connect(socket_fd, (SA *) &watchdog_addr, sizeof(watchdog_addr)) < 0) {

        printf("connection with the client failed \n");
        close(socket_fd);
        exit(-1);

    }

    fcntl(socket_fd, F_SETFL, O_NONBLOCK); /* Change the socket into non-blocking state	*/

////////////////////////////////////////////////////////////////////////////////////////////////

    struct timeval start,end;

    char buffer[1];

    int bytes_received=0;

    double timer=0;

    while(timer<10) {

        bytes_received = recv(socket_fd, &buffer, sizeof(buffer), 0);

        if (bytes_received>0) {

            if (strcmp(buffer, "k") == 0){ // that mean we get a response,  then update the timer to zero.

                timer = 0;
            }
        }
        else if (bytes_received==-1){


            timer++;
            sleep(1);

            if (timer == 10) {
                // if th timer equal to ten that mean we don't ger a echo response,
                //  then send a signal to close everything.


                send(socket_fd, "n", sizeof("n"), 0);

                close(socket_fd);
                exit(-1);

            }
            else { // until the timer dii from 10 , send a signal that say until know everything okay waiting a response.

                send(socket_fd, "m", sizeof("m"), 0);

            }
        }


    }

    return 0;
}