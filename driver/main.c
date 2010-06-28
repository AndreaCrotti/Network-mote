
// Includes
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <termios.h>

// Standard libraries
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Functions for using a tunnel device
#include "tun_dev.h"
// For adding an entry to the routing tables
#include "routing.h"
// Include the serial forwarder
#include <serialsource.h>

extern struct in6_addr __my_address;

// The IP address for our tunnel device
struct in6_addr __my_address       = {{{0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x65}}};

enum {
#ifndef __CYGWIN__
#ifndef LOSE32
  FALSE = 0,
  TRUE = 1,
#endif
#endif
  BUFSIZE = 256,
  MTU = 256,
};

struct serial_source_t {
  int fd;
  int non_blocking;
  void (*message)(serial_source_msg problem);

  /* Receive state */
  struct {
    uint8_t buffer[BUFSIZE];
    int bufpos, bufused;
    uint8_t packet[MTU];
    int in_sync, escaped;
    int count;
    struct packet_list *queue[256]; // indexed by protocol
  } recv;
  struct {
    uint8_t seqno;
    uint8_t *escaped;
    int escapeptr;
    uint16_t crc;
  } send;
};

/*************************/
/* Function declarations */
/*************************/
void stderr_msg(serial_source_msg problem);
void print_ip_packet(struct split_ip_msg *msg);
serial_source open_serial_source(const char *device, int baud_rate,
				 int non_blocking,
				 void (*message)(serial_source_msg problem));
static tcflag_t parse_baudrate(int requested);

int main(int args, char** arg) {
    char dev[IFNAMSIZ];
    char input[20];
    // Variables to pass to the serial forwarder
    /* char* sf_host = "127.0.0.1"; */
    /* int sf_port = 1000; */
    //Variables for a direct serial connection
    char *device = "/dev/ttyUSB0";
    int baud_rate = 115200;
    serial_source ser_src;

    // Open serial
    /* int ser_src = open_serial_source(argv[optind], platform_baud_rate(argv[optind + 1]), */
    /*                              1, stderr_msg); */
    ser_src = open_serial_source(device, baud_rate, 1, stderr_msg);
    if (!ser_src) {
      printf("Couldn't open serial port at device %s with baudrate %d\n", device, baud_rate);
      exit(1);
    }

    //Connect to mote using serial forwarder
    /* int sf_fd = open_sf_source(sf_host, sf_port); */
    /* if (sf_fd < 0) { */
    /*     printf("Couldn't connect to serial forwarder sf@%s:%d\n", sf_host, sf_port); */
    /*     exit(1); */
    /* } */

    // a new device should be opened!
    dev[0] = 0;    
    // create the tunnel
    int fd = tun_open(dev);
    if (fd < 1) {
      printf("Could not create tunnel device. Fatal.\n");
      return 1;
    } else {
      printf("created tun device: %s\n", dev);
    }

    //TODO: Ask the mote for it's IP address and set it

    // Setup the tunnel (Beneath other things, this sets the ip address)
    if (tun_setup(dev, &__my_address) < 0) {
      printf("configuring the tun failed; aborting\n");
      return 1;
    }

    //TODO: 
    
    // Testing...
    printf("IPV6_VERSION is: %d", IPV6_VERSION);
    fflush(stdout);

    // 

    uint8_t buf[sizeof(struct split_ip_msg) + INET_MTU];
    struct split_ip_msg *msg = (struct split_ip_msg *)buf;
    int len;

    while (1) {
        len = tun_read(fd, (void *)(&msg->pi), INET_MTU + sizeof(struct ip6_hdr));
        //if(len != -5)
            //printf("%d", len);
        

        if (len > 0) {
            printf("tun_read: read 0x%x bytes\n", len);

            /* if ((msg->hdr.vlfc[0] >> 4) != IPV6_VERSION) { */
            /*     printf("tun_read: discarding non-ip packet\n"); */
            /* } else */ 
            if (ntohs(msg->hdr.plen) > INET_MTU - sizeof(struct ip6_hdr)) {
                printf("tun_input: dropping packet due to length: 0x%x\n", ntohs(msg->hdr.plen));
            } else if (msg->hdr.nxt_hdr == 0) {
                printf("tun_input: dropping packet with IPv6 options\n");
            }
        }
        fflush(stdout);
        usleep(100);
    }

    tun_close(fd, dev);

    return 0;
}

void stderr_msg(serial_source_msg problem)
{
    //fprintf(stderr, "Note: %s\n", msgs[problem]);
    fprintf(stderr, "Note: Some error occurred when opening the serial port.\n");
}

/** 
 * Function to print out the contents of an IP packet.
 *
 * Taken from support/sdk/c/blib/driver/serial_tun.c
 * 
 * @param msg 
 */
void print_ip_packet(struct split_ip_msg *msg) {
  int i;
  struct generic_header *g_hdr;
  //if (log_getlevel() > LOGLVL_DEBUG) return;

  printf("  nxthdr: 0x%x hlim: 0x%x plen: %i\n", msg->hdr.nxt_hdr, msg->hdr.hlim, ntohs(msg->hdr.plen));
  printf("  src: ");
  for (i = 0; i < 16; i++) printf("0x%x ", msg->hdr.ip6_src.s6_addr[i]);
  printf("\n");
  printf("  dst: ");
  for (i = 0; i < 16; i++) printf("0x%x ", msg->hdr.ip6_dst.s6_addr[i]);
  printf("\n");

  g_hdr = msg->headers;
  while (g_hdr != NULL) {
    printf("header [%i]: ", g_hdr->len);
    for (i = 0; i < g_hdr->len; i++)
      printf("0x%x ", g_hdr->hdr.data[i]);
    printf("\n");
    g_hdr = g_hdr->next;
  }

  printf("data [%i]:\n\t", msg->data_len);
  for (i = 0; i < msg->data_len; i++) {
    if (i == 0x40) {
      printf (" ...\n");
      break;
    }
    printf("0x%x ", msg->data[i]);
    if (i % 16 == 15) printf("\n\t");
    if (i % 16 == 7) printf ("  ");
  }
  printf("\n");
}


serial_source open_serial_source(const char *device, int baud_rate,
				 int non_blocking,
				 void (*message)(serial_source_msg problem))
/* Effects: opens serial port device at specified baud_rate. If non_blocking
     is true, read_serial_packet calls will be non-blocking (writes are
     always blocking, for now at least)
   Returns: descriptor for serial forwarder at host:port, or
     NULL for failure (bad device or bad baud rate)
 */
{
#ifndef LOSE32
  struct termios newtio;
  int fd;
  tcflag_t baudflag = parse_baudrate(baud_rate);

  if (!baudflag)
    return NULL;

  fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd < 0)
    return NULL;

#ifdef __CYGWIN__
  /* For some very mysterious reason, this incantation is necessary to make
     the serial port work under some windows machines */
  HANDLE handle = (HANDLE)get_osfhandle(fd);
  DCB dcb;
  if (!(GetCommState(handle, &dcb) && SetCommState(handle, &dcb)))
    {
      close(fd);
      return NULL;
    }
#endif
  /* Serial port setting */
  memset(&newtio, 0, sizeof(newtio));
  newtio.c_cflag = CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR | IGNBRK;
  cfsetispeed(&newtio, baudflag);
  cfsetospeed(&newtio, baudflag);

  /* Raw output_file */
  newtio.c_oflag = 0;

  if (tcflush(fd, TCIFLUSH) >= 0 &&
      tcsetattr(fd, TCSANOW, &newtio) >= 0)
    {
        serial_source src = malloc(sizeof *src);

      if (src)
	{
	  memset(src, 0, sizeof *src);
	  src->fd = fd;
	  src->non_blocking = non_blocking;
	  src->message = message;
	  src->send.seqno = 37;

	  return src;
	}
    }
  close(fd);

  return NULL;
#else // LOSE32
	LPCTSTR       ComName = (LPCTSTR)device;
    HANDLE        hComm;
	DCB           dcb;
    serial_source src;

	int buflen = MultiByteToWideChar(CP_ACP,0,(PCSTR)device,-1,(LPWSTR)ComName,0);
	MultiByteToWideChar(CP_ACP,0,(PCSTR)device,-1,(LPWSTR)ComName,buflen);
	
	//syncronize
	hComm = CreateFile(ComName,  GENERIC_READ | GENERIC_WRITE,  0,  NULL,  OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL, NULL);

    if (hComm == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    PurgeComm(hComm, PURGE_RXCLEAR);

	GetCommState(hComm, &dcb); 
	dcb.BaudRate = baud_rate;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.fParity = FALSE;
	dcb.StopBits = ONESTOPBIT;
    if (SetCommState(hComm, &dcb) == 0) {
        return NULL;
    }

    src = malloc(sizeof *src);

    if (src) {
	  memset(src, 0, sizeof *src);
	  src->hComm = hComm;
	  src->non_blocking = non_blocking;
	  src->message = message;
	  src->send.seqno = 37;

	}

	return src;

#endif // LOSE32
}

#ifndef LOSE32
static tcflag_t parse_baudrate(int requested)
{
  int baudrate;

  switch (requested)
    {
#ifdef B50
    case 50: baudrate = B50; break;
#endif
#ifdef B75
    case 75: baudrate = B75; break;
#endif
#ifdef B110
    case 110: baudrate = B110; break;
#endif
#ifdef B134
    case 134: baudrate = B134; break;
#endif
#ifdef B150
    case 150: baudrate = B150; break;
#endif
#ifdef B200
    case 200: baudrate = B200; break;
#endif
#ifdef B300
    case 300: baudrate = B300; break;
#endif
#ifdef B600
    case 600: baudrate = B600; break;
#endif
#ifdef B1200
    case 1200: baudrate = B1200; break;
#endif
#ifdef B1800
    case 1800: baudrate = B1800; break;
#endif
#ifdef B2400
    case 2400: baudrate = B2400; break;
#endif
#ifdef B4800
    case 4800: baudrate = B4800; break;
#endif
#ifdef B9600
    case 9600: baudrate = B9600; break;
#endif
#ifdef B19200
    case 19200: baudrate = B19200; break;
#endif
#ifdef B38400
    case 38400: baudrate = B38400; break;
#endif
#ifdef B57600
    case 57600: baudrate = B57600; break;
#endif
#ifdef B115200
    case 115200: baudrate = B115200; break;
#endif
#ifdef B230400
    case 230400: baudrate = B230400; break;
#endif
#ifdef B460800
    case 460800: baudrate = B460800; break;
#endif
#ifdef B500000
    case 500000: baudrate = B500000; break;
#endif
#ifdef B576000
    case 576000: baudrate = B576000; break;
#endif
#ifdef B921600
    case 921600: baudrate = B921600; break;
#endif
#ifdef B1000000
    case 1000000: baudrate = B1000000; break;
#endif
#ifdef B1152000
    case 1152000: baudrate = B1152000; break;
#endif
#ifdef B1500000
    case 1500000: baudrate = B1500000; break;
#endif
#ifdef B2000000
    case 2000000: baudrate = B2000000; break;
#endif
#ifdef B2500000
    case 2500000: baudrate = B2500000; break;
#endif
#ifdef B3000000
    case 3000000: baudrate = B3000000; break;
#endif
#ifdef B3500000
    case 3500000: baudrate = B3500000; break;
#endif
#ifdef B4000000
    case 4000000: baudrate = B4000000; break;
#endif
    default:
      baudrate = 0;
    }
  return baudrate;
}
#endif
