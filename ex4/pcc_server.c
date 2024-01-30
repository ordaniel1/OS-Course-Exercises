#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>
#include <signal.h>

int connfd=-1;
int initial_pcc_global=0; //1 if pcc_global array was initialized
uint32_t pcc_global[95]; //to save the global statistics
uint32_t clientChars[96]; //counter of printable chars received by specific client. last cell - will contain C
int terminate=0; //1 if sigint was sent and we need to terminate program.




/*
 * function to handle the SIGINT signal according to the instructions.
 */
void my_signal_handler(int signum){
    int i;
    if (connfd==-1){ //no client is being processed - print statistics and terminate.
        if(initial_pcc_global == 1){
            for (i = 0; i < 95; i++) {
                printf("char '%c' : %u times\n", i + 32, pcc_global[i]);
            }
            exit(0);
        }
        else{ //we didn't even initialize pcc_global - print statistics with zeroes
            for (i = 0; i < 95; i++) {
                printf("char '%c' : %u times\n", i + 32, 0);
            }
            exit(0);
        }
    }
    else{ //client is being processed - turn on terminate.
        terminate=1;
    }
}




/*
 * function to register the SIGINT handling with the function above.
 */
int register_signal_handling(){
    struct sigaction new_action;
    memset(&new_action, 0, sizeof (new_action));
    new_action.sa_handler=my_signal_handler;
    return sigaction(SIGINT, &new_action, NULL);

}



int main(int argc, char *argv[]) {

    //register signal handling
    if(register_signal_handling()==-1){
        perror("Signal handle registration failed");
        exit(1);
    }
    int i;
    int TCP_error_flag; //TCP error indicator

    //initialize pcc_global
    memset(pcc_global, 0, sizeof(pcc_global));
    initial_pcc_global=1;  // global characters counter was initialized

    //validate number of arguments
    if (argc!=2){
        perror("invalid number of arguments");
        exit(1);
    }


    //set server
    int flag=1; //for setsockopt
    char* p; //for strtoul
    int listenfd=-1;

    struct sockaddr_in serv_addr;
    struct sockaddr_in peer_addr;

    socklen_t addrsize=sizeof(struct sockaddr_in);

    listenfd= socket(AF_INET, SOCK_STREAM,0);
    if (listenfd<0){
        perror("Couldn't create a socket");
        exit(1);
    }
    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (int))<0){
        perror("setsockopt failed");
        exit(1);
    }

    memset(&serv_addr,0,addrsize);

    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr= htonl(INADDR_ANY);
    serv_addr.sin_port= htons(strtoul(argv[1], &p, 10)); //set server's port
    if(0!=bind(listenfd, (struct sockaddr*)&serv_addr, addrsize)){
        perror("bind failed");
        exit(1);
    }
    if (0!=listen(listenfd,10)){
        perror("listen failed");
        exit(1);
    }

    while(1){
        TCP_error_flag=0; //no TCP error yet

        //check if termination is required
        if(terminate == 1){ //there was a sigint, we need to terminate
            for(i=0; i<95; i++){ //print statistics
                printf("char '%c' : %u times\n", i + 32, pcc_global[i]);
            }
            exit(0);
        }

        //accept a client
        connfd= accept(listenfd, (struct sockaddr*)&peer_addr, &addrsize);
        if (connfd<0){
            perror("Server couldn't accept A TCP connection");
            exit(1);
        }



        //receive N from client
        uint32_t N; //will contain N - that the server will receive from client
        uint32_t num; //helper variable
        char* dataN= (char*)&num; //we want to read  N from client into array
        int not_read=sizeof (num); //number of bytes that haven't been read yet
        int total_read=0; //number of bytes that have been read.
        int nread; //number of bytes the have been read in each iteration.


        while (total_read < sizeof(num)){
            nread=read(connfd, dataN + total_read, not_read);
            if (nread<0){ //there is an error with reading
                if (errno==ETIMEDOUT|| errno== ECONNRESET || errno==EPIPE){ //if TCP error, continue to next client
                    close(connfd);
                    connfd=-1;
                    TCP_error_flag=1;
                    perror("TCP error occurred while reading data from client");
                    break;
                }
                else{ //if there is other error - exit
                    perror("Couldn't read data from client ");
                    exit(1);
                }
            }
            if (nread==0){ //indicates that client process was killed unexpectedly - continue
                close(connfd);
                connfd=-1;
                TCP_error_flag=1;
                perror("client process was killed unexpectedly");
                break;
            }

            //update total_read and not_read
            total_read+=nread;
            not_read-=nread;

        }

        if (TCP_error_flag==1){
            continue;
        }




        N= ntohl(num); //convert num to uint_32t


        char* N_bytes_buffer=malloc(N); //allocate memory for the N bytes read from client
        if (N_bytes_buffer == NULL){
            perror("Server code couldn't allocate memory");
        }



        //receive N bytes from client
        not_read=N; //number of bytes that haven't been read yet
        total_read=0; //number of bytes that have been read.
        while (total_read<N){
            nread=read(connfd, N_bytes_buffer + total_read, not_read);
            if (nread<0){ //there is an error with reading
                if (errno==ETIMEDOUT|| errno== ECONNRESET || errno==EPIPE){ //if TCP error, continue to next client
                    free(N_bytes_buffer);
                    close(connfd);
                    connfd=-1;
                    TCP_error_flag=1;
                    perror("TCP error occurred while reading data from client");
                    break;
                }
                else{ //if there is other error - exit
                    perror("Couldn't read data from client ");
                    exit(1);
                }
            }
            if (nread==0){ //indicates that client process was killed unexpectedly - continue
                free(N_bytes_buffer);
                close(connfd);
                connfd=-1;
                TCP_error_flag=1;
                perror("client process was killed unexpectedly");
                break;
            }

            //update total_read and not_read
            total_read+=nread;
            not_read-=nread;

        }
        if (TCP_error_flag==1){
            continue;
        }


        //count printable characters
        int index;
        memset(clientChars, 0, sizeof(clientChars)); //initialize clientChars array
        for (i=0; i<N; i++){
            if((32 <= N_bytes_buffer[i]) && (N_bytes_buffer[i] <= 126)){
                index= (int)N_bytes_buffer[i] - 32;
                clientChars[index]++;
                clientChars[95]++; //update C
            }
        }
        free(N_bytes_buffer);


        //send C to the client
        uint32_t covertC= htonl(clientChars[95]);
        char* data=(char*)&covertC;
        int not_written=sizeof (covertC); //number of bytes that haven't been written yet
        int total_written=0;  //number of bytes that have been written.
        int written; //number of bytes the have been written in each iteration.
        while(total_written<sizeof (covertC)){
            written=write(connfd, data+total_written, not_written);
            if (written<0) { //there is an error with writing
                if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE) {//if TCP error, continue to next client
                    close(connfd);
                    connfd = -1;
                    TCP_error_flag=1;
                    perror("TCP error occurred while sending C to client");
                    break;
                } else { //if there is other error - exit
                    perror("Couldn't send C to client ");
                    exit(1);
                }

            }

            //update values
            total_written+=written;
            not_written-=written;

        }

        if (TCP_error_flag==1){
            continue;
        }



        //update pcc_global
        for(i=0; i<95; i++){
            pcc_global[i]+=clientChars[i];
        }


        //close connection
        close(connfd);
        connfd=-1;





    }


}



