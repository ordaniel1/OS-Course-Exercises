#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[]) {
    FILE* fp;
    char ch;
    uint32_t N=0;
    char* p;
    char* fileData;

    //validate number of arguments
    if (argc!=4){
        perror("Invalid number of arguments");
        exit(1);
    }

    //open file
    fp = fopen(argv[3], "r");
    if (fp == NULL) {
        perror("couldn't open file");
        exit(1);
    }

    //find N
    while((ch=fgetc(fp))!=EOF){
         N++;
    }
    rewind(fp);

    //read the N bytes of Data from file
    fileData= malloc(N); //allocate memory to save the data
    if (fileData==NULL){
        perror("Couldn't allocate memory for save file's data");
        exit(1);
    }

    if( fread(fileData, sizeof(char), N, fp)!=N){ //read from file to fileData buffer
        perror("Couldn't read N bytes from file");
    }

    fclose(fp); //close file


    //set client
    int sockfd=-1;
    struct sockaddr_in serv_addr;


    if( (sockfd= socket(AF_INET, SOCK_STREAM, 0))<0){
        perror("Couldn't create a socket");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof (serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port= htons(strtoul(argv[2], &p, 10));
    inet_pton(AF_INET, argv[1], &(serv_addr.sin_addr));



    //connect to the server
    if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))<0){
        perror("Couldn't connect to server");
        exit(1);
    }




    //send N to the server
     uint32_t covertN= htonl(N);
    char* data=(char*)&covertN;
    int not_written=sizeof (covertN);
    int total_written=0;
    int written;
    while(total_written<sizeof (covertN)){
        written=write(sockfd, data+total_written, not_written);
        if(written<0){
            perror("Couldn't send the number N to the server");
            exit(1);
        }
        total_written+=written;
        not_written-=written;

    }


    //send N bytes of data to the server
    total_written=0;
    not_written=N;
    while(total_written<N){
        written=write(sockfd, fileData+total_written,not_written);
        if(written<0){
            perror("Couldn't send N bytes of data to the server");
            exit(1);
        }


        total_written+=written;
        not_written-=not_written;
    }
    free(fileData); //free allocated memory


    //receive C from server
    uint32_t num;
    uint32_t C;
    char* dataC=(char*)&num;
    int not_read=sizeof(num);
    int total_read=0;
    int nread;
    while(total_read<sizeof (num)){
        nread=read(sockfd,dataC+total_read, not_read);
        if(nread<0){ //if there is an error with read
            perror("Couldn't receive C from the server");
            exit(1);
        }

        //update values
        total_read+=nread;
        not_read-=nread;
    }

    C= ntohl(num);
    close(sockfd);
    printf("# of printable characters: %u\n",C);
    exit(0);





}

