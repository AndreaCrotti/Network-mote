// Standard libraries
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "motecomm.h"
#include "chunker.h"

// Functions for using a tunnel device
#include "tunnel.h"

#include "motecomm.h"
#include "glue.h"
#include "setup.h"

// a wrapper for mcp::receive that will be understood by the fdglue module
void serialReceive(fdglue_handler_t* that) {
    mcp_t* this = (mcp_t*)(that->p);
    this->getComm(this)->read(this->getComm(this));
}

// call the script and give error if not working
void callScript(char *script_cmd, char *success, char *err_msg, int is_fatal) {
    // Run the setup script for the tunnel
    int err = system(script_cmd);
    if (err != 0) {
        perror(err_msg);
    } else {
        printf(success);
        if (is_fatal) {
            exit(1);
        }
    }
}

void tunReceive(int client_no, fdglue_handler_t* that) {
    printf("tunReceive called\n");
    
    struct TunHandlerInfo* this = (struct TunHandlerInfo*)(that->p);
    static stream_t buf[MAX_FRAME_SIZE];
    memset(buf,0,MAX_FRAME_SIZE);
    int size = tunRead(client_no, (char*)buf,MAX_FRAME_SIZE);
    assert(size);
    static int seqno = 0;
    ++seqno;
    payload_t payload = {.stream = buf, .len = size};
    ipv6Packet ipv6;
    unsigned sendsize = 0;
    int no_chunks = needed_chunks(size);
    char chunks_left;

    do {
        chunks_left = genIpv6Packet(&payload, &ipv6, &sendsize, seqno, no_chunks);
        assert(sendsize);
        
        printf("Sending chunk with size %u\n", sendsize);
        unsigned counter = sendsize;
        unsigned char *char_data = (unsigned char*)&ipv6;
        while(counter--){
            printf("%02X ", (unsigned)*char_data++);
        }
        printf("\n");
        
        // Use the raw packet for now...
        //this->ifp->send(this->ifp,(payload_t){.stream = (stream_t*)&ipv6, .len = sendsize});
        this->mcomm->send(this->mcomm, (payload_t){.stream = (stream_t*)&ipv6, .len = sendsize});
        
        sendsize = 0;
    } while (chunks_left); 
}

la_t localAddress = DEFAULT_LOCAL_ADDRESS;

void laSet(laep_handler_t* this, la_t const address) {
    (void)this;
    localAddress = address;
}
