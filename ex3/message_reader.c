#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include "message_slot.h"



int main(int argc, char* argv[]) {
    int fd;
    int ret_val;
    unsigned int channel_id;
    char *p;
    char buf[128];

    //check the number of arguments
    if (argc!=3){
        perror("invalid number of arguments");
        exit(1);
    }



    //open the specified message slot device file
    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
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

    //read the message
    ret_val =read(fd, buf, 128);
    if(ret_val<0) {
        perror("read failed");
        exit(1);
    }

    //close the device
    close(fd);

    //print the message to standard output
    ret_val=write(STDOUT_FILENO, buf, ret_val);
    if (ret_val<0){
        perror("couldn't write to standard output");
        exit(1);
    }

    exit(0);


    }