#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include "../setup.h"

char msg[] = "hallo hallo\n";
#define TUN_DEV "/dev/net/tun"
char ipaddr[] = "10.0.0.1";
char setup_script[] = "./test_tun_iptables.sh";

int create_tun(char *dev) {
    struct ifreq ifr;
    int err;
    char *clonedev = TUN_DEV;

    // open the tun device rw mode
    int fd = open(clonedev , O_RDWR);
    if (fd < 0) {
        perror("Opening /dev/net/tun");
        exit(1);
    }
    
    // prepare ifr
    memset(&ifr, 0, sizeof(ifr));
    // Set the flags
    ifr.ifr_flags = IFF_TUN;

    // If a device name was specified it is put to ifr
    if (*dev) {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    // Try to create the device
    if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
        close(fd);
        perror("Creating the device");
        return err;
    }

    // Write the name of the new interface to device
    strcpy(dev, ifr.ifr_name);
    printf("successfully created %s\n", dev);

    return fd;
}

int write_tun_write(int fd) {
    return write(fd, msg, strlen(msg));
}

int read_from(int fd, char *buf, int len) {
    return read(fd, buf, len);
}

/* int write_with_socket(int sock) { */
/*     int sd = socket(PF_INET,sdfsdfw SOCK_RAW, IPPROTO_UDP); */
    
/*     return 0; */
/* } */ 

int main() {
    char name[IFNAMSIZ];
    name[0] = 0;    
    int fd = create_tun(name);
    
    // Setting up IP-tables routes
    char script_call[strlen(setup_script) + 1 + IFNAMSIZ];
    strncpy(script_call, setup_script, strlen(setup_script) + 1);
    strncpy(script_call + strlen(setup_script), " ", 1);
    strncpy(script_call + strlen(setup_script) + 1, name, IFNAMSIZ);
    printf("Calling script with %s\n", script_call); 
    fflush(stdout);
    callScript(script_call, "Created iptable entries\n", 
               "An error occured during creaton of iptable entries", 0);

    for(;;){
        int wrote = write_tun_write(fd);
        printf("wrote %d bytes on the tun device, now check iptables log\n", wrote);
        usleep(500000);
    }

    char buf[strlen(msg) + 1];
    
    // read is blocking here 
    // read_from(fd, buf, strlen(msg));
    printf("reading from the device string\n");
    return 0;
}
