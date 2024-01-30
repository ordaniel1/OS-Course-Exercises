#include <stdio.h>
#include "os.h"




void destroy_mapping(uint64_t pt, uint64_t vpn) {
    int i;
    uint64_t partial_vpn,currNodePPN,currNodeAddress,x,pte;
    void *virtualNodeAddress;
    x=511; //we'll use it to take relevant 9 bits of vpn
    currNodePPN=pt;

    for (i = 0; i < 5; i++) {
        partial_vpn = (vpn>>(36-9*i))&x; //the relevant part of the vpn to the current level
        currNodeAddress=(currNodePPN<<12); //get a full physical address of node by adding zeros
        virtualNodeAddress=(uint64_t*)(phys_to_virt(currNodeAddress)); //get the virtual address of the current node
        if (virtualNodeAddress == NULL){
            return;
        }
        pte=*(((uint64_t*)(virtualNodeAddress)) + partial_vpn); //the relevant pte
        if ((pte&1)==0) { //stop - there is no map
            return;
        }

        if (i<4) {  //take the ppn of the node in the next level
            currNodePPN=pte>>12;
        }

        if(i==4){ //last level - update the validity bit in the last level to be invalid.
            *(((uint64_t*)(virtualNodeAddress)) + partial_vpn)= pte - 1;
        }

    }
}





void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn){
    if(ppn==NO_MAPPING){ //destroy
        destroy_mapping(pt,vpn);
    }
    else{ //update or create
        int i;
        uint64_t partial_vpn, currNodePPN, currNodeAddress, x, pte, frame;
        uint64_t* virtualNodeAddress;
        x=511; //we'll use it to take relevant 9 bits of vpn
        currNodePPN=pt;

        for (i=0; i<5; i++){
            partial_vpn=(vpn>>(36-9*i))&x; //the relevant part of the vpn to the current level
            currNodeAddress=(currNodePPN<<12);//get a full physical address of node by adding zeros
            virtualNodeAddress=phys_to_virt(currNodeAddress); //get the virtual address of the current node
            pte=*(((uint64_t*)(virtualNodeAddress)) + partial_vpn); //the relevant pte
            if(((pte&1)==0)&&(i<4)){ //if invalid
                frame=alloc_page_frame(); //alloc a physical page and get it's ppn
                *(((uint64_t*)(virtualNodeAddress)) + partial_vpn)= (frame << 12) + 1; //update the pte
            }

            if(i<4){ //take the ppn of the node in the next level
                currNodePPN=(*(((uint64_t*)(virtualNodeAddress)) + partial_vpn) >> 12);

            }
            if (i==4){ //last level - update the pte with the given ppn
                *(((uint64_t*)(virtualNodeAddress)) + partial_vpn)= (ppn << 12) + 1;
            }
        }

    }
}

uint64_t page_table_query(uint64_t pt, uint64_t vpn) {
    int i;
    uint64_t partial_vpn, currNodePPN, currNodeAddress, x, pte;
    void *virtualNodeAddress;
    x = 511;
    currNodePPN = pt; //physical page number
    for(i=0; i<5; i++) {
        partial_vpn=(vpn>>(36-9*i))&x; //the relevant part of the vpn to the current level
        currNodeAddress=(currNodePPN<<12); //get a full physical address of node by adding zeros
        virtualNodeAddress=(uint64_t*)(phys_to_virt(currNodeAddress)); //get the virtual address of the current node
        if (virtualNodeAddress == NULL) {
            return NO_MAPPING;
        }
        pte=*(((uint64_t*)(virtualNodeAddress)) + partial_vpn); //the relevant pte
        if((pte&1)==0){ //if invalid
            return NO_MAPPING;
        }
        currNodePPN=pte>>12; //take the ppn of the node in the next level

    }
    return (pte>>12); //return the ppn saved in the pte in the last level - the ppn that the vpn mapped to
}
