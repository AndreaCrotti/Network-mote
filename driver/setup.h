#include "glue.h"
#include "motecomm.h"

struct TunHandlerInfo {
    int client_no;
    ifp_t* ifp;
    motecomm_t* mcomm;
};


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
