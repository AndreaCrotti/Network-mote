// measures in bytes
#define SIZE_IPV6_HEADER 40
#define MAX_CARRIED 127
#define TOT_PACKET_SIZE(payload_len) (sizeof(ip6_hdr) + sizeof(myPacketHeader) + payload_len)

typedef struct in6_addr in6_src;
typedef struct in6_addr in6_dst;

// also the internal struct should be packed
struct myPacketHeader {
    uint8_t seq_no;
    uint8_t ord_no;
    unsigned short checksum;
} __attribute__((__packed__));

typedef struct myPacketHeader myPacketHeader;

// only the final ipv6 packet must be "__packed__".
struct ipv6Packet {
    struct ip6_hdr ip6_hdr;
    myPacketHeader packetHeader;
    void *payload;
} __attribute__((__packed__));

typedef struct ipv6Packet ipv6Packet;

// just for more ease of writing
typedef struct sockaddr_in6 sockaddr_in6;
typedef struct ip6_hdr ip6_hdr;

ipv6Packet *genIpv6Packets(void *, int, int);

void *reconstruct(ipv6Packet *, int);

unsigned short csum(unsigned short *, int);

void dataToLocalhost(void *, int, int);

ip6_hdr *genIpv6Header(size_t);

sockaddr_in6 *localhostDest(void);

void testWithMemset(void);

void sendToLocalhost(void *, size_t);

void sendDataTo(void *, struct sockaddr *, size_t, int);
