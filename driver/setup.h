/**
 * Bundled functionality for both gateway and client.
 *
 * @author Andrea Crotti, Marius Grysla, Oscar Dustmann
 */
#include "glue.h"
#include "motecomm.h"

// interval between two transmissions in micro seconds
// this value was roughly determined by testing smaller values may work
// this value may depend on the topology.
#define SERIAL_INTERVAL_US 40000

/**
 * If there comes in something from the tun, this struct provides information for the handler, how to process it.
 */
struct Tun_handler_info {
    int client_no;
    motecomm_t* mcomm;
};

/**
 * as mainy as it gets - everything is started from here
 *
 * @param fdg The fdglue_t object to use: it must have already been configured (@see init_glue)
 */
void main_loop(fdglue_t *fdg);

/**
 * Set the local address.
 */
void la_set(laep_handler_t* this, la_t const address);

/**
 * Invoked by the glue module when something comes in from the tun fd.
 */
void tun_receive(fdglue_handler_t* that);

/**
 * Invoked by the glue module when something comes in from the serial fd.
 */
void serial_receive(fdglue_handler_t* that);

/** 
 * Call a shell script and check its result
 * 
 * @param script_cmd command to send
 * @param success print in case of success
 * @param err print in case of error
 * @param is_fatal exit if failing and 1, only error otherwise
 */
void call_script(char *script_cmd, char *success, char *err, int is_fatal);

/**
 * Initialise the glue interface to listen both the tun and serial fds.
 *
 * @param g The glue object to use.
 * @param sif The serial interface to use. This is required to access the fd to listen on.
 * @param mcp The mcp object to use with incoming packets.
 * @param client_no An abstraction of file descriptors in order to support different clients (TODO).
 */
void init_glue(fdglue_t* g, serialif_t* sif, mcp_t* mcp, int client_no);

/// helper functions to set up different serial connections
serialif_t *create_serial_connection(char const *dev, mcp_t **mcp);
serialif_t *create_sf_connection(char const* host, char const* port, mcp_t **mcp);
serialif_t *create_fifo_connection(mcp_t** _mcp);

/** 
 * Processing data from the serial interface
 * 
 * @param that 
 * @param payload 
 */
void serial_process(struct motecomm_handler_t *that, payload_t const payload);
