#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MINREQUESTINPUTS 4    /* Minimum number of arguments for a request */
#define MAXREQUESTSIZE 100    /* Maximum number of a bytes of a request */
#define FIRSTARG 3            /* The first argument in argv */  
#define RETURNED_ERROR -1
#define MAX_PORT_NO 65535     /* Port Range */
#define MIN_PORT_NO 0           
#define HELP_REQ "--help"
#define HELP "Usage: controller <address> <port> {[-o out_file] [-log log_file][-t seconds] <file> [arg...] | mem [pid] | memkill <percent>}"
#define OUT "-o"
#define LOG "-log"

/*****************************************************************************************
 * function:                Converts the client's request into a char array and sends the 
 *                          request to the overseer 
 * param socket_id:         the port number the request is made on.
 * param inputs:            the number of inputs specified by the client.
 * param request:           An array of strings containing the client's input arguments.
 * 
 * **************************************************************************************/
void SendControllerRequest(int socket_id, int inputs, const char *request[]){
    
    char  *reqStr;
    reqStr = (char *) malloc(MAXREQUESTSIZE);
    memset(reqStr, 0, MAXREQUESTSIZE);
    int reqSize = 0;

    for(int i = 0; i < inputs; i++){
        strcat(reqStr, request[i]);
        reqSize =  reqSize + strlen(request[i]) + 1;
        strcat(reqStr, " ");
    }

    /*Convert Request Size to network byte order. */ 
    uint32_t reqSizeNB = htonl(MAXREQUESTSIZE);    
    if(send(socket_id, reqStr, reqSizeNB, 0) == RETURNED_ERROR){
        fprintf(stdout, "%s\n", HELP);
    }

    free(reqStr);
} 
/*****************************************************************************************
 * function:                Entrypoint of the controller.
 * 
 * **************************************************************************************/
int main(int argc, char *argv[])
{
    int sockfd;
    struct hostent *he;
    struct sockaddr_in their_addr; /* connector's address information */

    /* Check if Program is the only input */
    if(argc == 1){
        fprintf(stderr, "%s\n", HELP);
        exit(1);
    }

    /* Check for help request */
    if(strcmp(argv[1], HELP_REQ) == 0){
        fprintf(stdout, "%s\n", HELP);
        exit(1);
    }

    /* Check minimum number of requests */
    if(argc < MINREQUESTINPUTS){
        fprintf(stderr, "%s\n", HELP);
        exit(1);
    }
    
    /* Get Hostname and Validate */
    if ((he = gethostbyname(argv[1])) == NULL)
    {
        fprintf(stdout, "%s\n", HELP);
        exit(1);
    }
    /* Get Socket No. and Validate */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == RETURNED_ERROR)
    {
        fprintf(stdout, "%s\n", HELP);
        exit(1);
    }

    /* clear address struct */
    memset(&their_addr, 0, sizeof(their_addr));

    their_addr.sin_family = AF_INET;            /* host byte order */
    their_addr.sin_port = htons(atoi(argv[2])); /* short, network byte order */
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);

    /* Check Port No. is within a valid range */
     if(atoi(argv[2]) > MAX_PORT_NO || atoi(argv[2]) < MIN_PORT_NO){
        fprintf(stderr, "%s\n", HELP);
        exit(1);
    } 
    
    /* Check Correct number of inputs for out or log when specified */ 
     if(((strcmp(argv[3], OUT) == 0) || (strcmp(argv[3], LOG) == 0)) && argc < 6){
        fprintf(stderr, "%s\n", HELP);
        exit(1);
    }  

    /* Confirm -o is always before -log */
     for(int i = 3; i < argc - 1; i++){
        if(strcmp(argv[i],LOG) == 0){
            for(int j = i+1; j < argc; j++){
                if(strcmp(argv[j], OUT) == 0){
                    fprintf(stderr, "%s\n", HELP);
                    exit(1);
                } 
            }
            i = argc;
        }
    } 
    if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == RETURNED_ERROR){
        fprintf(stderr, "Could not connect to overseer at %s %d \n", inet_ntoa(their_addr.sin_addr), atoi(argv[2]));
        exit(1);
    }
    /* Make a copy of arguments to pass to overseer */
    int argCount = argc - FIRSTARG; 
    const char *inputArray[argCount];              
    for(int i = 0; i < argCount; i++){
        inputArray[i] = argv[i + FIRSTARG];
    }
    
    SendControllerRequest(sockfd, argCount, inputArray);

    close(sockfd);
    return 0;
}