// Standard libraries
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h> // usleep

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
    (void)that;
    //printf("hey I got something from the mote! p is %p\n",that->p);
    addChunk(payload);
}

// TODO: here there should be all the logic of the gateway/client
void mainLoop() {
    
}

// call the script and give error if not working
void callScript(char *script_cmd, char *success, char *err_msg, int is_fatal) {
    // Run the setup script for the tunnel
    int err = system(script_cmd);
    if (err != 0) {
        perror(err_msg);
        if (is_fatal) {
            exit(1);
        }
    } else {
        printf("%s",success);
    }
}

void reconstructDone(payload_t complete) {
  /*printf("RECONSTRUCT DONE!\tsize: %u\n",complete.len);
  for (unsigned i = 0; i < complete.len; i++) {
    printf("%02X ",(unsigned)complete.stream[i]);
  }
  printf("\n");*/
  /* debug start */ {
    unsigned sum = 0;
    for (stream_t* p = (stream_t*)complete.stream; p - (stream_t*)complete.stream < (signed)complete.len; p++) {
      sum += *p;
    }
    static unsigned recv_count = 0;
    printf(" => Checksum of RECV %u packet is %08X\n",recv_count++,sum);
  } /* debug end */
  tunWriteNoQueue(/*FIXME*/ 0 /*FIXME*/,complete);
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

    initReconstruction(reconstructDone);

    if (*mcp) {
        printf("Connection to %s over device %s opened.\n", mote, dev);
    } else {
        printf("There was an error opening the connection to %s over device %s.\n", mote, dev);
    }
    return sif;
}

// receiving data from the tunnel device
void tunReceive(fdglue_handler_t* that) {
    //printf("tunReceive called\n");
    
    struct TunHandlerInfo* this = (struct TunHandlerInfo*)(that->p);
    static stream_t buf[MAX_FRAME_SIZE];
    memset(buf, 0, MAX_FRAME_SIZE);
    int size = tunRead(this->client_no, (char*)buf,MAX_FRAME_SIZE);
    assert(size);
    static int seqno = 0;
    ++seqno;
    payload_t payload = {.stream = buf, .len = size};

    /* debug start */ {
    unsigned sum = 0;
    for (stream_t* p = buf; p - buf < size; p++) {
      sum += *p;
    }
    static unsigned sent_count = 0;
    printf(" <= Checksum of SENT packet %u is %08X\n",sent_count++,sum);
    } /* debug end */

    ipv6Packet ipv6;
    unsigned sendsize = 0;
    int no_chunks = neededChunks(size);
    char chunks_left;
    char first = 1;

    // generate all the subchunks and send them out
    do {
        if (!first) {
          usleep(SERIAL_INTERVAL_US);
        }
        first = 0;
        chunks_left = genIpv6Packet(&payload, &ipv6, &sendsize, seqno, no_chunks);
        assert(sendsize);
        //printf("Sending ord_no: %u (seq_no: %u)\n",(unsigned)ipv6.header.packetHeader.ord_no, (unsigned)ipv6.header.packetHeader.seq_no);
        
        /*printf("Sending chunk with size %u\n", sendsize);
        unsigned counter = sendsize;
        unsigned char *char_data = (unsigned char*)&ipv6;
        while(counter--){
            printf("%02X ", (unsigned)*char_data++);
        }
        printf("\n");*/
        
        payload_t to_send = {
            .stream = (stream_t*)&ipv6,
            .len = sendsize
        };

        this->mcomm->send(this->mcomm, to_send);
        sendsize = 0;

    } while (chunks_left); 
}

la_t localAddress = DEFAULT_LOCAL_ADDRESS;

void laSet(laep_handler_t* this, la_t const address) {
    (void)this;
    localAddress = address;
}
