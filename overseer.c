#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <libgen.h>

#define MIN_INPUTS 2        /* The minimum number of inputs for the overseer to start listening. */
#define MAXREQUESTSIZE 100  /* Maximum number of a bytes of a request. */
#define BACKLOG 10          /* how many pending connections queue will hold */ 
#define RETURNED_ERROR -1   
#define FORMAT_YEAR 1900    /* Formatting year value for time.h */
#define FORMAT_MONTH 1      /* Formatting month value for time.h */

/*****************************************************************************************
 * function:                Receives a request from controller and returns it as a char
 *                          array.
 * 
 * param socket_identifier: the port number the request is made on.
 * **************************************************************************************/
char *ReceiveControllerRequest(int socket_identifier){
    int reqSize = 0;
    char *reqStr;
    reqStr = (char *) malloc(MAXREQUESTSIZE);
    memset(reqStr, 0, MAXREQUESTSIZE);
    
    if((reqSize = recv(socket_identifier, reqStr, MAXREQUESTSIZE, 0)) == RETURNED_ERROR){
        perror("recv");
        exit(EXIT_FAILURE);
    }

    if(reqSize > MAXREQUESTSIZE){
        printf("Request Size Exceeded");
        exit(1);
    }
    return reqStr;
}

/*****************************************************************************************
 * function:                Determines the number of input arguments made by the client
 *                          Request.
 * 
 * param str:               A char array containing the client request.
 * param delimeter:         A char array containing a delimter character used to determine
 *                          seperations between arguments. 
 * **************************************************************************************/
int GetInputCount(char str[], char delimeter[]){
    int delimeterCount = 0;
    char *pch;
    
    pch = strpbrk(str, delimeter);
    while(pch != NULL){
        delimeterCount++;
        pch = strpbrk(pch+1, delimeter);
    }
    
    return delimeterCount;
}
/*****************************************************************************************
 * function:                formats the client request and executes it via execv
 * 
 * param request:           A char array containing the client request.
 * param delimeter:         A char array containing a delimter character used to determine
 *                          seperations between arguments.
 * param argCount:          The number of arguments found in the client request
 * 
 * **************************************************************************************/
void ProcessControllerRequest(char *request, char *delimeter, int argCount){
    char *argPtr, *fileName; 
    char *filePath;
    filePath = (char*) malloc(MAXREQUESTSIZE);
    char reqCopy[MAXREQUESTSIZE];
    strcpy(reqCopy, request);
      
    /* Get the file path and file name */
    argPtr = strtok(reqCopy, delimeter);
    strcpy(filePath, argPtr);
    fileName = basename(filePath);

    /* Fill Argument Array */
    char *arguments[argCount];
    arguments[0] = fileName;
    if(argCount > 1){
        for(int i = 1; i < argCount; i++){
            argPtr = strtok(NULL, delimeter);
            arguments[i] = argPtr;
        }
    }
    arguments[argCount] = NULL;

    execv(filePath, arguments);
}

/*****************************************************************************************
 * function:                Entrypoint of the overseer.
 * 
 * **************************************************************************************/
int main(int argc, char *argv[])
{
    int sockfd, new_fd;            /* listen on sock_fd, new connection on new_fd */
    struct sockaddr_in my_addr;    /* my address information */
    struct sockaddr_in their_addr; /* connector's address information */
    socklen_t sin_size; 

    char *delimeter = " ";

    /* Initialise time values */
    time_t rawTime = time(NULL);
    if (rawTime == RETURNED_ERROR){
        perror("time");
    }
    struct tm  *LogTime= localtime(&rawTime);
  
    /* Get port number for server to listen on */
    if (argc != MIN_INPUTS)
    {
        fprintf(stderr, "usage: port_number\n");
        exit(1);
    }

    /* generate the socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == RETURNED_ERROR)
    {
        perror("socket");
        exit(1);
    }

    /* Enable address/port reuse, useful for server development */
    int opt_enable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt_enable, sizeof(opt_enable));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &opt_enable, sizeof(opt_enable));

    /* clear address struct */
    memset(&my_addr, 0, sizeof(my_addr));

    /* generate the end point */
    my_addr.sin_family = AF_INET;            /* host byte order */
    my_addr.sin_port = htons(atoi(argv[1])); /* short, network byte order */
    my_addr.sin_addr.s_addr = INADDR_ANY;    /* auto-fill with my IP */

    /* bind the socket to the end point */
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == RETURNED_ERROR)
    {
        perror("bind");
        exit(1);
    }

    /* start listnening */
    if (listen(sockfd, BACKLOG) == RETURNED_ERROR)
    {
        perror("listen");
        exit(1);
    }

    printf("server starts listening ...\n");

    /* repeat: accept, send, close the connection */
    /* for every accepted connection, use a sepetate process or thread to serve it */
    while (1)
    { /* main accept() loop */

        sin_size = sizeof(struct sockaddr_in);
        if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr,
                             &sin_size)) == RETURNED_ERROR)
        {
            perror("accept");
            continue;
        }

        /* Call method to recieve receive request from client */
        char * requestString;
        requestString= (char*) malloc(MAXREQUESTSIZE);
        requestString = ReceiveControllerRequest(new_fd);

        time(&rawTime);
        LogTime = localtime(&rawTime);
        fprintf(stdout, "%d-%02d-%02d %02d:%02d:%02d - connection received from %s\n", LogTime->tm_year + FORMAT_YEAR, LogTime->tm_mon + FORMAT_MONTH, LogTime->tm_mday, LogTime->tm_hour, LogTime->tm_min, LogTime->tm_sec, inet_ntoa(their_addr.sin_addr));
        
        pid_t process = fork();

        /* this is the child process */
        if (process == 0){
            
            /* Print out the request sent by client */
            time(&rawTime);
            LogTime = localtime(&rawTime);
            fprintf(stdout, "%d-%02d-%02d %02d:%02d:%02d - attempting to execute %s\n", LogTime->tm_year + FORMAT_YEAR, LogTime->tm_mon + FORMAT_MONTH, LogTime->tm_mday, LogTime->tm_hour, LogTime->tm_min, LogTime->tm_sec, requestString);

            /* Process Request */
            int argumentCount = GetInputCount(requestString, delimeter);
            ProcessControllerRequest(requestString, delimeter, argumentCount);
            exit(1);
        }
        
        else{
            int status;
            wait(&status);
            close(new_fd);
            time(&rawTime);
            LogTime = localtime(&rawTime);
            
            /*Program Failed */
            if(WEXITSTATUS(status) == 1) {
                fprintf(stdout, "%d-%02d-%02d %02d:%02d:%02d - could not execute %s\n", LogTime->tm_year + FORMAT_YEAR, LogTime->tm_mon + FORMAT_MONTH, LogTime->tm_mday, LogTime->tm_hour, LogTime->tm_min, LogTime->tm_sec, requestString);
            }
            
            /* Program Success */
            else{
                fprintf(stdout, "%d-%02d-%02d %02d:%02d:%02d - %s has been executed with pid %d\n", LogTime->tm_year + FORMAT_YEAR, LogTime->tm_mon + FORMAT_MONTH, LogTime->tm_mday, LogTime->tm_hour, LogTime->tm_min, LogTime->tm_sec, requestString, process);
                fprintf(stdout, "%d-%02d-%02d %02d:%02d:%02d - %d has terminated with status code %d\n", LogTime->tm_year + FORMAT_YEAR, LogTime->tm_mon + FORMAT_MONTH, LogTime->tm_mday, LogTime->tm_hour, LogTime->tm_min, LogTime->tm_sec, process, WEXITSTATUS(status));
            }
            free(requestString);
        }
        while (waitpid(RETURNED_ERROR, NULL, WNOHANG) > 0)
            ; /* clean up child processes */
    }
    return 0;
}