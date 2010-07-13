/*******************************************************************************/
/* tunnel.c                                                                    */
/*                                                                             */
/* This file contains a new and simple implementation for the tun/tap drivers  */
/* WITH COMMENTS!                                                              */
/*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>  
#include <net/route.h>
#include <signal.h>

#include "tunnel.h"

// Variables to store a route configuration
char stored_gw = 0;
struct rtentry saved_entry;
// The name of the interface 
char *ifname;

// Storage for our socket identifier
int sock = 0;

/** 
 * Creates a new tun/tap device, or connects to a already existent one depending
 * on the arguments.
 * 
 * @param dev The name of the device to connect to or '\0' when a new device should be
 *            should be created.
 * @param flags IFF_TUN of IFF_TAP, depending on whether tun or tap should be used.
 *              Additionally IFF_NO_PI can be set.
 * 
 * @return Error-code.
 */
int tun_open(char *dev, int flags){
    
    struct ifreq ifr;
    int fd, err;
    char *clonedev = "/dev/net/tun";
    
    // Open the clone device
    if( (fd = open(clonedev , O_RDWR)) < 0 ) {
        perror("Opening /dev/net/tun");
        return fd;
    }
    
    // prepare ifr
    memset(&ifr, 0, sizeof(ifr));
    // Set the flags
    ifr.ifr_flags = flags;

    // If a device name was specified it is put to ifr
    if(*dev){
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    // Try to create the device
    if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ){
        close(fd);
        perror("Creating the device");
        return err;
    }

    // Write the name of the new interface to device
    strcpy(dev, ifr.ifr_name);

    // allocate 10 Bytes for the interface name
    ifname = malloc(10);

    return fd;
}

// TODO: remove all the stuff ifconfig-related, will be done in a script instead
/** 
 * This function is used to restore the gateway before the program stops. 
 */
void restore_gateway(int param){
    (void) param;

    int fd, err; 

    // Getting the device identifier with the socket command
    if( (fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("Getting socket");
    }

    printf("Restoring the standard route over '%s'\n", saved_entry.rt_dev);

    // Restore the old gateway
    if( (err = ioctl(fd, SIOCADDRT, &saved_entry)) < 0){
        perror("Restoring routing entry");
    }

    exit(0);
}

/** 
 * Sets up the tunnel interface and assigns a MTU and a IPv4 address.
 * 
 * @param dev The device name.
 * @param addr An IPv4 address (As a string).
 * 
 * @return Error-code.
 */
int tun_setup(char *dev, char *addr){
    
    struct ifreq ifr;
    struct rtentry rte;
    struct sockaddr_in sock_addr;
    int fd, err;
    (void)fd;
    int mtu = 1280;
    
    // TODO: what do we need a socket for when we setup the device??
    // Getting the device identifier with the socket command
    if( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("Getting socket");
        return sock;
    }

    // Prepare the ifr struct
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    // Set up the interface
    ifr.ifr_flags |= IFF_UP;
    if( (err = ioctl(sock, SIOCSIFFLAGS, &ifr)) < 0) {
        perror("Setting up the interface");
        return err;
    }

    // Assign the MTU value
    ifr.ifr_mtu = mtu;
    if( (err = ioctl(sock, SIOCSIFMTU, &ifr)) < 0) {
        perror("Assigning MTU");
        return err;
    }
    
    // Reset Ifr and set the name and family
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name,dev,IFNAMSIZ);
    ifr.ifr_addr.sa_family = AF_INET;

    // Set the IP-addres
    struct sockaddr_in *inaddr = (struct sockaddr_in *)&ifr.ifr_addr;
    inet_aton(addr, &inaddr->sin_addr);
    if( (err = ioctl(sock, SIOCSIFADDR, &ifr)) < 0) {
        perror("Assigning IP");
        return err;
    }

    //// TEMPORARY ////
    FILE *in=popen("netstat -rn", "r");
    char tmp[256]={0x0};
    int i;
    char *gw_str, *genm_str, *flag_str, *mss_str, *window_str, *irrt_str, *interface_str;
    for(i = 0; fgets(tmp,sizeof(tmp),in)!=NULL; i++){
        // Ignore the first two lines
        if(i >= 2){
            // Cut the string into tokens
            char *dest_str = strtok(tmp, " ");
            // Check if we got the standard gateway
            if(strcmp(dest_str, "0.0.0.0") == 0){
                gw_str = strtok(NULL, " ");
                genm_str = strtok(NULL, " ");
                flag_str = strtok(NULL, " ");
                mss_str = strtok(NULL, " ");
                window_str = strtok(NULL, " ");
                irrt_str = strtok(NULL, " ");
                interface_str = strtok(NULL, "\n");
                // Routing table has a gateway
                stored_gw = 1;
            }
            
        }
    }
    pclose(in);
 
    // Store the data in the global rtentry struct
    if(stored_gw){
        struct in_addr p_addr;
        memset(&p_addr, 0, sizeof(p_addr));

        memset(&saved_entry, 0, sizeof(saved_entry));
        // Destination
        sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_port = 0;
        memcpy(&saved_entry.rt_dst, &sock_addr, sizeof(sock_addr));
        // Gateway
        inet_aton(gw_str, &p_addr);
        //inet_aton(gw_str, &sock_addr.sin_addr);
        sock_addr.sin_addr.s_addr = htonl(p_addr.s_addr);
        memcpy(&saved_entry.rt_gateway, &sock_addr, sizeof(sock_addr));
        // Network Mask
        inet_aton(genm_str, &p_addr);
        sock_addr.sin_addr.s_addr = htonl(p_addr.s_addr);
        memcpy(&saved_entry.rt_genmask, &sock_addr, sizeof(sock_addr));
        // Flags ('R' is not supported yet ...) 
        unsigned short int flags = 0;
        if(strchr(flag_str, 'U')) flags |= RTF_UP;
        if(strchr(flag_str, 'H')) flags |= RTF_HOST;
        if(strchr(flag_str, 'G')) flags |= RTF_GATEWAY;
        //if(strchr(flag_str, 'R')) flags |= RTF_REJECT;
        if(strchr(flag_str, 'D')) flags |= RTF_DYNAMIC;
        if(strchr(flag_str, 'M')) flags |= RTF_MODIFIED;
        if(strchr(flag_str, '!')) flags |= RTF_REJECT;
        saved_entry.rt_flags = htonl(flags);
        // mss value
        saved_entry.rt_mss = htonl(atoi(mss_str));
        // Window
        saved_entry.rt_window = htonl(atoi(window_str));
        // irrt value
        saved_entry.rt_irtt = htonl(atoi(irrt_str));
        // interface name
        strncpy(ifname, interface_str, 10);
        saved_entry.rt_dev = ifname;
    }

  
    // Delete the current standard entry
    if(stored_gw){
        memset(&rte, 0, sizeof(struct rtentry));
        memset(&sock_addr, 0, sizeof(sock_addr));
        // Build up the address (we want to delete the default gateway)
        sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_port = 0;
        memcpy(&rte.rt_dst, &sock_addr, sizeof(sock_addr));
        // Mask is also default
        memcpy(&rte.rt_genmask, &sock_addr, sizeof(sock_addr));
        // As is the gateway
        memcpy(&rte.rt_gateway, &sock_addr, sizeof(sock_addr));
        if( (err = ioctl(sock, SIOCDELRT, &rte)) < 0){
            perror("Deleting routing entry");
            return err;
        }
    }

    // Add a routing entry
    memset(&rte, 0, sizeof(struct rtentry));
    // Set destination to default (INADDR_ANY)
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = 0;
    
    memcpy(&rte.rt_dst, &sock_addr, sizeof(sock_addr));
    // Mask is also default
    memcpy(&rte.rt_genmask, &sock_addr, sizeof(sock_addr));
    // As is the gateway
    memcpy(&rte.rt_gateway, &sock_addr, sizeof(sock_addr));
    
    rte.rt_metric = 15;
    rte.rt_dev = dev;
    rte.rt_flags = RTF_UP;

    if( (err = ioctl(sock, SIOCADDRT, &rte)) < 0){
        perror("Adding routing entry");
        return err;
    }
    
    if(stored_gw){
        // restore_gateway should be called if the program stops
        signal(SIGINT, restore_gateway);
    }

    return 0;
}

/** 
 * Reads data from the tunnel and exits if a error occurred.
 * 
 * @param fd The tunnel device.
 * @param buf This is where the read data are written.
 * @param length maximum number of bytes to read.
 * 
 * @return number of bytes read.
 */
int tun_read(int fd, char *buf, int length){
    
    int nread;

    if((nread=recv(fd, buf, length, 0)) < 0){
        perror("Reading data");
        exit(1);
    }
    return nread;
}

/** 
 * Writes data from buf to the device.
 * 
 * @param fd The tunnel device.
 * @param buf Pointer to the data.
 * @param length Maximal number of bytes to write.
 * 
 * @return number of bytes written.
 */
int tun_write(int fd, char *buf, int length){
    int nwrite;
    
    //TODO: Maybe send is better here
    if((nwrite=send(fd, buf, length, 0)) < 0){
        perror("Writing data");
        exit(1);
    }
    return nwrite;
}
