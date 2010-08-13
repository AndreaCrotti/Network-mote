#include "glue.h"
#include "motecomm.h"

// interval between two transmissions in micro seconds
// this value was roughly determined by testing smaller values may work
// this value may depend on the topology.
#define SERIAL_INTERVAL_US 200000

struct Tun_handler_info {
    int client_no;
    motecomm_t* mcomm;
};

void main_loop(fdglue_t *fdg);

void la_set(laep_handler_t* this, la_t const address);

void tun_receive(fdglue_handler_t* that);

/** 
 * Call a shell script and check its result
 * 
 * @param script_cmd command to send
 * @param success print in case of success
 * @param err print in case of error
 * @param is_fatal exit if failing and 1, only error otherwise
 */
void call_script(char *script_cmd, char *success, char *err, int is_fatal);

void serial_receive(fdglue_handler_t* that);

void init_glue(fdglue_t* g, serialif_t* sif, mcp_t* mcp, int client_no);

serialif_t * create_serial_connection(char const *dev, mcp_t **mcp);
serialif_t *create_sf_connection(char const* host, char const* port, mcp_t **mcp);
serialif_t* create_fifo_connection(mcp_t** _mcp);

/** 
 * Processing data from the serial interface
 * 
 * @param that 
 * @param payload 
 */
void serial_process(struct motecomm_handler_t *that, payload_t const payload);
