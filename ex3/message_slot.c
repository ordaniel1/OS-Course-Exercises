#undef __KERNEL__
#define __KERNEL__

#undef MODULE
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>
#include "message_slot.h"

MODULE_LICENSE("GPL");



struct channel{
    unsigned int id; //channel id
    unsigned int messageLength; //length of the message that saved in the channel
    char* data; // points to the message
    struct channel* next; //points to the next channel
};


struct message_slot_file{
    int minor; //minor number
    struct channel *channel; //each 'message_slot_file' structure will have a linked list of channel
    struct message_slot_file *next; //points to the next 'message_slot_file' structure
};

typedef struct message_slot_file file_node;
typedef struct channel channel_node;

file_node HEAD={-1,NULL,NULL}; //the 'ROOT' of the structures



//==========================functions for maintain structures==================================

file_node* findFile(file_node* head, unsigned int minor){ //find file_node structure, or create one.
                                                            // save them as "linked list".
    file_node* curr=head;
    while(curr->next!=NULL){ //find node in a linked list and return it
        curr=curr->next;
        if(curr->minor==minor){
            return curr;
        }
    }

    //there is no file node with the given minor number, so we need to create one.
    curr->next=(file_node*)kmalloc(sizeof(file_node), GFP_KERNEL); //allocate memory for the file_node
    if(curr->next==NULL){ //if there is a memory allocation error - return NULL
        return NULL;
    }
    curr=curr->next; //now curr points to the new file_node
    curr->minor=minor; //initial Fields
    curr->next=NULL;
    curr->channel=NULL;
    return curr; //return a pointer to the file node
}





channel_node* findChannel(file_node* node, unsigned long id){ //find channel_node structure, or create one.
                                                                // save them as "linked list".
    channel_node* curr=node->channel;
    if(curr==NULL){ //if the file has no channels yet -create the first channel
        node->channel=(channel_node*)kmalloc(sizeof (channel_node), GFP_KERNEL); //allocate memory for the channel_node
        if(node->channel==NULL){  //if there is a memory allocation error - return NULL
            return NULL;
        }

        curr=node->channel; //now curr points to the new channel_node
        curr->id=id; //initial fields
        curr->messageLength=0;
        curr->data=NULL;
        curr->next=NULL;
        return curr; //return a pointer to the channel_node
    }
    //else -file_node has already some channels - find the desired one or create it, if no exists.
    if(curr->id==id){ //if it's the first channel_node - return a pointer
        return curr;
    }

    while(curr->next!=NULL){ //else - keep look for the channel
        curr=curr->next;
        if(curr->id==id){
            return curr;
        }
    }

    //if there is no channel_node with the given id - create one.
    curr->next=(channel_node*)kmalloc(sizeof (channel_node), GFP_KERNEL); //allocate memory for the channel_node
    if(curr->next==NULL){ //if there is a memory allocation error - return NULL
        return NULL;
    }
    curr=curr->next; //now curr points to the new channel_node
    curr->id=id; //initial fields
    curr->messageLength=0;
    curr->data=NULL;
    curr->next=NULL;
    return curr; //return a pointer to the channel_node

}





void freeChannels(channel_node* head){ //free memory of channels that were created for a file_node as a linked list
    channel_node* curr=head;
    channel_node* temp;
    while (curr->next!=NULL){
        if((curr->data)!=NULL){
            kfree(curr->data);
        }
        temp=curr;
        curr=curr->next;
        kfree(temp);
    }
    if((curr->data)!=NULL) {
        kfree(curr->data);
    }
    kfree(curr);

}





void freeFiles(file_node* head){ //free memory of file_nodes that were created as a linked list
    file_node* curr=head->next;
    file_node* temp;
    while(curr!=NULL){
        temp=curr;
        curr=curr->next;
        if ((temp->channel)!=NULL){
            freeChannels(temp->channel); //free channels_nodes that connected to the file_node
        }
        kfree(temp);

    }
}


//=================================DEVICE METHODS==================================


static long device_ioctl(struct file* file, unsigned int ioctl_command_id, unsigned long ioctl_param){
    if(ioctl_command_id==MSG_SLOT_CHANNEL){ //if the given command is the right one
        if(ioctl_param>0){ //if the given channel's id (ioctl_param) is a positive integer
            printk("Invoking ioctl: setting channel id to %ld\n", ioctl_param);
            file->private_data=(void*)ioctl_param; //save the channel's id in private_data field of file* structure
            return 0;
        }
    }
    return -EINVAL; //any other case is an ERROR - invalid arguments.
}


static int device_open( struct inode* inode, struct file* file){
    unsigned int minor;
    printk("Invoking device_open(%p)\n", file);
    minor=iminor(inode); //get the minor number
    findFile(&HEAD, minor); //create file_node structure, if not exists.
    return 0;

}





static ssize_t device_write(struct file*  file, const char __user* buffer,
                            size_t length, loff_t* offset){
    int i;
    unsigned int minor;
    file_node* currFile;
    channel_node* channel;

    if (length>128 || length==0){ //if message is too long, or zero length
        return -EMSGSIZE;
    }

    if (buffer==NULL){
        return -EINVAL;
    }

    if ((file->private_data)==NULL){ //no channel has been set
        return -EINVAL;
    }

    minor=iminor(file->f_path.dentry->d_inode); //find minor number
    currFile= findFile(&HEAD, minor); //find file's structure
    if(currFile==NULL){ //memory allocation error
        return -ENOMEM;
    }

    channel= findChannel(currFile, (unsigned long)(file->private_data)); //find the channel
    if(channel==NULL){ //memory allocation error
        return -ENOMEM;
    }

    printk("Invoking device_write(%p,%ld)\n", file, length);


    if((channel->data)!=NULL){ //free an old message in the channel, if exists
        kfree(channel->data);
        channel->messageLength=0;
    }

    channel->data=(char*)kmalloc(sizeof(char)*length,GFP_KERNEL); //allocate memory for the new message
    if(channel->data==NULL){ //memory allocation error
        return -ENOMEM;
    }

    for (i=0; i<length && i< BUF_LEN; ++i){ //write data from buffer to the channel
        get_user((channel->data)[i], &buffer[i]);
    }

    if(i!=length){ //incorrect number of bytes has been written
        kfree(channel->data); //free the memory of the message
        i=0; //set i=0 - so messageLength field in channel's structure will be set to zero
    }

    channel->messageLength=i; //set messageLength to be the length of the recent message in channel.
    return i; //return number of bytes that has been written.
}




static ssize_t device_read(struct file*  file,  char __user* buffer,
                            size_t length, loff_t* offset){
    int i;
    unsigned int minor;
    file_node* currFile;
    channel_node* channel;

    if ((file->private_data)==NULL){ //no channel has been set
        return -EINVAL;
    }

    if(buffer==NULL){
        return -EINVAL;
    }

    printk("Invoking device_read(%p,%ld)\n", file, length);

    minor=iminor(file->f_path.dentry->d_inode); //get the minor number
    currFile= findFile(&HEAD, minor);  //find file's structure
    if(currFile==NULL){ //memory allocation error
        return -ENOMEM;
    }

    channel= findChannel(currFile, (unsigned long)(file->private_data)); //find the channel
    if(channel==NULL){ //memory allocation error
        return -ENOMEM;
    }

    if(channel->messageLength==0){ //there is no message in the channel
        return -EWOULDBLOCK;
    }

    if ((channel->messageLength)>length){ //buffer isn't large enough
        return -ENOSPC;
    }


    for(i=0; i< (channel->messageLength); i++){ //read from channel to the buffer
        put_user(channel->data[i], &buffer[i]);
    }

    if (i!=(channel->messageLength)){ //couldn't read the whole message
        return -EFAULT;
    }
    return i; //return correct number of bytes that were read.
}





static int device_release(struct inode* inode, struct file* file){
    printk("Invoking device_release(%p,%p)\n", inode, file);
    file->private_data=NULL;
    return 0;
}



//====================================DEVICE SETUP===================================

struct file_operations Fops={
        .owner  = THIS_MODULE,
        .unlocked_ioctl  = device_ioctl,
        .open   = device_open,
        .read   = device_read,
        .write  = device_write,
        .release= device_release
};




static int __init slot_init(void){

    int major=-1;
    major=register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops);
    if (major<0){ //couldn't register (with 240 as a major number)
        printk(KERN_ERR "%s registration failed for %d\n",
               DEVICE_FILE_NAME, MAJOR_NUM);
        return major;
    }
    printk("Hello Kernel world\n"); //success
    return 0;
}







static void __exit slot_cleanup(void){
    freeFiles(&HEAD); //free all the memory that were allocated
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME); //unregister
    printk("cleaning up module\n");
}


module_init(slot_init);
module_exit(slot_cleanup);







