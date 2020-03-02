#include <sys/wait.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <netdb.h>
#include <signal.h>
#include "helpers.h"

#define BUFFER_SIZE 1024

struct sockaddr_in server;
struct hostent *hostnm;
int sockfd, id, port, numbytes, PID;
char buffer[BUFFER_SIZE], userinput[BUFFER_SIZE];

void end_session();

int main(int argc, char *argv[]) {

    signal(SIGINT, end_session);

    if(argc != 3) {
        fprintf(stderr, "Useage: %s <IP address> <Port number>\n", argv[0]);
        exit(1);
    }

    // Get the server address
    if((hostnm = gethostbyname(argv[1])) == NULL) {
        perror("Failed to get hostname");
        exit(1);
    }

    // Get the port number
    port = atoi(argv[2]);

    // Get socket file descriptor
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Failed to create socket");
        exit(1);
    }

    // Specify server address
    server.sin_family = AF_INET;
    server.sin_addr = *((struct in_addr *)hostnm->h_addr);
    server.sin_port = htons(port);

    // Connect to the server
    if(connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Failed to connect");
        exit(1);
    }

    // Create 2 client processes for sending and recieveing
    if((PID = fork()) >= 0) {
        /*
         *  This process follows user input and sends data to server
         */
        if(PID == 0) {
            while(1) {
                // Place text into buffer (userinput) until newline charcater (Enter)
                fgets(userinput, BUFFER_SIZE, stdin);
                if(userinput[0] != '\0') {
                    // Send userinput to the server
                    if((numbytes = send(sockfd, userinput, sizeof(userinput), 0)) == -1) {
                        perror("Failed to send message");
                        exit(1);
                    }
                    else if(numbytes == 0) {
                        printf("Connection to server has been terminated\n");
                        exit(0);
                    }
                    // Clear the buffer for future use
                    memset(userinput, 0, sizeof(userinput));
                }
            }
        }
        /*
         *  This process recieves and displays data from the server
         */
        else {
            while(1) {
                if((numbytes = recv(sockfd, buffer, sizeof(buffer), 0)) == -1) {
                    perror("Failed to receive message");
                    exit(1);
                }
                else if(numbytes == 0) {
                    printf("Connection to server has been terminated\n");
                    end_session();
                }
                // If theres anything in the buffer print it to screen
                if(buffer[0] != '\0') {
                    printf("\n%s\n\n", buffer);
                }
                memset(buffer, 0, sizeof(buffer));
            }
        }
    }
    else {
        perror("Failed to fork process");
        exit(1);
    }
}

// Terminate the client session
void end_session() {
    send(sockfd, "BYE", 3, 0);
    // Close client file descriptor and kill the process
    shutdown(sockfd, SHUT_RDWR);

    if(kill(PID, SIGKILL) < 0) {
        perror("Kill process");
        exit(1);
    }
    wait(NULL);
    if(kill(getpid(), SIGKILL) < 0) {
        perror("Kill process");
        exit(1);
    }
}
