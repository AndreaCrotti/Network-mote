// Standard libraries
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "motecomm.h"
#include "chunker.h"

// Functions for using a tunnel device
#include "tunnel.h"
#include "reconstruct.h"
#include "motecomm.h"
#include "glue.h"
#include "setup.h"

// a wrapper for mcp::receive that will be understood by the fdglue module
void serialReceive(fdglue_handler_t* that) {
    mcp_t* this = (mcp_t*)(that->p);
    this->getComm(this)->read(this->getComm(this));
    
}

// function to overwrite the handler and process data from serial 
void serialProcess(struct motecomm_handler_t *that, payload_t const payload) {
    (void) that;
    addChunk(payload);
}

// call the script and give error if not working
void callScript(char *script_cmd, char *success, char *err_msg, int is_fatal) {
    // Run the setup script for the tunnel
    int err = system(script_cmd);
    if (err != 0) {
        perror(err_msg);
    } else {
        printf("%s",success);
        if (is_fatal) {
            exit(1);
        }
    }
}

serialif_t *createSerialConnection(char const *dev, mcp_t **mcp) {
    char mote[] = "telosb";
    serialif_t *sif = NULL;

    *mcp = openMcpConnection(dev, mote, &sif);
    // at the moment we're not using these things
    /* ifp(0, mcp); */
    /* laep(0, mcp); */
    /* _laep.setHandler(laep(0, mcp), LAEP_REPLY,(laep_handler_t) {.handle = laSet, .p = NULL}); */
    // XXX HACK:
    motecomm_t* mc = (*mcp)->getComm(*mcp);
    mc->setHandler(mc,(motecomm_handler_t) {
      .p = 0,
      .receive = serialProcess
    });

    if (*mcp) {
        printf("Connection to %s over device %s opened.\n", mote, dev);
    } else {
        printf("There was an error opening the connection to %s over device %s.\n", mote, dev);
    }
    return sif;
}

void tunReceive(fdglue_handler_t* that) {
    printf("tunReceive called\n");
    
    struct TunHandlerInfo* this = (struct TunHandlerInfo*)(that->p);
    static stream_t buf[MAX_FRAME_SIZE];
    memset(buf, 0, MAX_FRAME_SIZE);
    int size = tunRead(this->client_no, (char*)buf,MAX_FRAME_SIZE);
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
        
        this->mcomm->send(this->mcomm, (payload_t){.stream = (stream_t*)&ipv6, .len = sendsize});
        
        sendsize = 0;
    } while (chunks_left); 
}

la_t localAddress = DEFAULT_LOCAL_ADDRESS;

void laSet(laep_handler_t* this, la_t const address) {
    (void)this;
    localAddress = address;
}
