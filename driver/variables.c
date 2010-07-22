#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "variables.h"

#define FD_NOT_SET (-1)
#define TUN_NAME_NOT_SET ""

static int tun_fd = FD_NOT_SET;
// use another size
static char tun_name[10] = TUN_NAME_NOT_SET;

int getFd() {
    return tun_fd;
}

void getTunName(char *name) {
    strcpy(name, tun_name);
}

void setFd(int fd) {
    assert(tun_fd == FD_NOT_SET);
    tun_fd = fd;
}

void setTunName(char *name) {
    assert(!strcmp(tun_name, TUN_NAME_NOT_SET));
    strcpy(tun_name, name);
}
