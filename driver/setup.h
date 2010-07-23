#include "glue.h"
#include "motecomm.h"

struct TunHandlerInfo {
    int fd;
    ifp_t* ifp;
    motecomm_t* mcomm;
};


void laSet(laep_handler_t* this, la_t const address);

void tunReceive(int client_no, fdglue_handler_t* that);
void callScript(char *script_cmd, char *success, char *err);
void serialReceive(fdglue_handler_t* that);
