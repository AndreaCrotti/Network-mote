#include "glue.h"
#include "motecomm.h"

// interval between two transmissions in micro seconds
// this value was roughly determined by testing smaller values may work
// this value may depend on the topology.
// => FIXME
#define SERIAL_INTERVAL_US 40000

struct TunHandlerInfo {
    int client_no;
    motecomm_t* mcomm;
};

void main_loop(fdglue_t *fdg);

void laSet(laep_handler_t* this, la_t const address);

void tunReceive(fdglue_handler_t* that);

/** 
 * Call a shell script and check its result
 * 
 * @param script_cmd command to send
 * @param success print in case of success
 * @param err print in case of error
 * @param is_fatal exit if failing and 1, only error otherwise
 */
void callScript(char *script_cmd, char *success, char *err, int is_fatal);

void serialReceive(fdglue_handler_t* that);

serialif_t * createSerialConnection(char const *dev, mcp_t **mcp);
serialif_t *createSfConnection(char const* host, char const* port, mcp_t **mcp);

/** 
 * Processing data from the serial interface
 * 
 * @param that 
 * @param payload 
 */
void serialProcess(struct motecomm_handler_t *that, payload_t const payload);
