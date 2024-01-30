#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include "message_slot.h"



int main(int argc, char* argv[]){

    int fd;
    int ret_val;
    long length;
    unsigned int channel_id;
    char* p;

    //check the number of arguments
    if (argc!=4){
        perror("invalid number of arguments");
        exit(1);
    }

    //open the specified message slot device file
    fd=open(argv[1], O_WRONLY);
    if (fd<0){
        perror("couldn't open file");
        exit(1);
    }


    //convert arg[2] to int
    channel_id =strtoul(argv[2], &p, 10);

    //set the channel id
    ret_val= ioctl(fd,MSG_SLOT_CHANNEL,channel_id);
    if (ret_val<0){
        perror("ioctl failed");
    }

    //find message's length
    length= strlen(argv[3]);

    //write the specified message to the message slot device file
    ret_val=write(fd, argv[3],length);
    if(ret_val!=length){
        perror("write failed");
        exit(1);
    }

    close(fd);
    exit(0);
}




