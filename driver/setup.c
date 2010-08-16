#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "motecomm.h"
#include "chunker.h"

#include "tunnel.h"
#include "reconstruct.h"
#include "motecomm.h"
#include "glue.h"
#include "setup.h"
#include "compress.h"

char* tun_active;

serialif_t* sif_used;

void serial_buffer_full(void) {
    *tun_active = 0;
}

void serial_buffer_empty(void) {
    *tun_active = 1;
}

void _close_everything(int param) {
    LOG_DEBUG("closing all open file descriptors");
    (void)param; // param only useful for signal prototype
    
    // closing tunnel and compression module
    close_all_tunnels();
    close_compression();
    exit(EXIT_SUCCESS);
}

void init_glue(fdglue_t* g, serialif_t* sif, mcp_t* mcp, int client_no) {
    tun_active = NULL;
    
    fdglue(g);

    // structures for the handlers, it's an event driven program
    // so we need to setup handlers
    struct Tun_handler_info* thi = malloc(sizeof(struct Tun_handler_info));
    thi->client_no = client_no;
    thi->mcomm = mcp->get_comm(mcp);

    fdglue_handler_t hand_sif = {
        .p = mcp,
        .handle = serial_receive
    };
    fdglue_handler_t hand_thi = {
        .p = thi,
        .handle = tun_receive
    };

    g->set_handler(g, sif->fd(sif), FDGHT_READ, hand_sif, FDGHR_APPEND,NULL);
    g->set_handler(g, get_fd(client_no), FDGHT_READ, hand_thi, FDGHR_APPEND, &tun_active);

    sif_used = sif;
}

void main_loop(fdglue_t *fdg) {
    // when receiving SIGINT calling the _close_everything function
    signal(SIGINT, _close_everything);
    LOG_DEBUG("Initialize the compression module");
    init_compression();

    unsigned lcount = 0;
    (void)lcount;
    assert(sif_used);
    for (;;) {
        LOG_INFO("listening %d ...",lcount++);
        print_statistics();
        // NOTE: this is just an arbitrary sleep interval
        // if we wait for 5 minutes and did not receive a single packet,
        // this loop will just reiterate (printing stats and so on)
        fdg->listen(fdg, 60, 0);        
    }
}

// a wrapper for mcp::receive that will be understood by the fdglue module
void serial_receive(fdglue_handler_t* that) {
    mcp_t* this = (mcp_t*)(that->p);
    this->get_comm(this)->read(this->get_comm(this));
}

// function to overwrite the handler and process data from serial 
void serial_process(struct motecomm_handler_t *that, payload_t const payload) {
    (void)that;
    //LOG_DEBUG("hey I got something from the mote! p is %p",that->p);
    add_chunk(payload);
}

// call the script and give error if not working
void call_script(char *script_cmd, char *success, char *err_msg, int is_fatal) {
    // Run the setup script for the tunnel
    int err = system(script_cmd);
    if (err != 0) {
        LOG_ERROR("Could not execute script %s", script_cmd);
        perror(err_msg);
        if (is_fatal) {
            exit(1);
        }
    } else {
        LOG_INFO("%s",success);
    }
}

void reconstruct_done(payload_t complete) {
    LOG_DEBUG("reconstruct done\tsize: %u",complete.len);
    unsigned sum = 0;
    for (stream_t* p = (stream_t*)complete.stream; p - (stream_t*)complete.stream < (signed)complete.len; p++) {
        sum += *p;
    }
    static unsigned recv_count = 0;
    LOG_NOTE(" => Checksum of RECV %u packet is %08X", recv_count++, sum);

    tun_write(DEFAULT_CLIENT_NO, complete);
}

serialif_t *create_serial_connection(char const *dev, mcp_t **mcp) {
    char mote[] = "telosb";
    serialif_t *sif = NULL;

    *mcp = open_mcp_connection(dev, mote, &sif);
    // XXX HACK:
    motecomm_t* mc = (*mcp)->get_comm(*mcp);
    mc->set_handler(mc,(motecomm_handler_t) {
            .p = 0,
                .receive = serial_process
                });

    init_reconstruction(reconstruct_done);

    if (*mcp) {
        LOG_INFO("Connection to %s over device %s opened.", mote, dev);
    } else {
        LOG_ERROR("There was an error opening the connection to %s over device %s.", mote, dev);
        exit(1);
    }
    return sif;
}

serialif_t *create_sf_connection(char const* host, char const* port, mcp_t **_mcp) {
    char _port[strlen(port)+1];
    strcpy(_port,port);
    serialif_t* sif = serialforwardif(NULL,host,_port);
    assert(sif);

    motecomm_t* mc = motecomm(NULL,sif);
    *_mcp = mcp(NULL, mc);
    mc->set_handler(mc,(motecomm_handler_t) {
            .p = 0,
                .receive = serial_process
                });

    init_reconstruction(reconstruct_done);

    if (*_mcp) {
        LOG_INFO("Connection to %s over port %s opened.", host, _port);
    } else {
        LOG_ERROR("There was an error opening the connection to %s over device %s.", host, _port);
        exit(1);
    }
    return sif;
}

/// a fake - or dummy - connection to an application running on the same machine
serialif_t* create_fifo_connection(mcp_t** _mcp) {
    serialif_t* sif = serialfakeif(NULL);
    assert(sif);
    motecomm_t* mc = motecomm(NULL,sif);
    *_mcp = mcp(NULL, mc);

    mc->set_handler(mc,(motecomm_handler_t) {
            .p = 0,
                .receive = serial_process
                });

    init_reconstruction(reconstruct_done);

    if (*_mcp) {
        LOG_INFO("Fake connection over stdin/stdout opened.");
    } else {
        LOG_ERROR("There was an error opening the fake connection over stdin/stdout.");
        exit(1);
    }
    return sif;
}

// receiving data from the tunnel device
void tun_receive(fdglue_handler_t* that) {
    struct Tun_handler_info* this = (struct Tun_handler_info*)(that->p);
    // allocated only once and always reused!!
    static stream_t buf[MAX_FRAME_SIZE];
    memset(buf, 0, MAX_FRAME_SIZE);
    int size = tun_read(this->client_no, (char*)buf,MAX_FRAME_SIZE);
    assert(size);
    static seq_no_t seqno = 0;
    ++seqno;
    payload_t payload = {
        .stream = buf,
        .len = size,
        .is_compressed = false
    };

#if COMPRESSION_ENABLED
    // replace the payload with another payload
    static stream_t compr_data[MAX_FRAME_SIZE];
    payload_t compressed = {
        .len = MAX_FRAME_SIZE,
        .stream = compr_data,
    };
    
    // we'll overwrite it when done
    payload_compress(payload, &compressed);
    // overwrite the current value ONLY if has to be compressed
    if (compressed.len < payload.len) {
        LOG_DEBUG("enabling compression");
        print_gained(payload.len, compressed.len);
        // should we alloc - memcpy - free instead?
        copy_payload(&compressed, &payload);
        size = payload.len;
        payload.is_compressed = true;
    } else {
        LOG_DEBUG("compression disabled, non compressible data");
    }
            
#endif
    {
        unsigned sum = 0;
        if (DEBUG) {
            for (stream_t* p = buf; p - buf < size; p++) {
                sum += *p;
            }
        }
        static unsigned sent_count = 0;
        LOG_NOTE("<= Checksum of SENT packet %u is %08X",sent_count++,sum);
    }

    my_packet pkt;
    unsigned sendsize = 0;
    int no_chunks = needed_chunks(size);
    char chunks_left;

    // generate all the subchunks and send them out
    do {
        usleep(SERIAL_INTERVAL_US);
        chunks_left = gen_packet(&payload, &pkt, &sendsize, seqno, no_chunks);
        assert(sendsize);
        LOG_DEBUG("Sending ord_no: %u (seq_no: %u)",(unsigned)pkt.packet_header.ord_no, (unsigned)pkt.packet_header.seq_no);
        
        payload_t to_send = {
            .stream = (stream_t*)&pkt,
            .len = sendsize
        };

        this->mcomm->send(this->mcomm, to_send);
        sendsize = 0;

    } while (chunks_left);
}
