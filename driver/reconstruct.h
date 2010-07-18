// max number of packets kept in the temporary structure for reconstruction
#define MAX_RECONSTRUCTABLE 10

// this could be computed somehow from the MAX_ETHERNET and the size carried
#define MAX_CHUNKS 100

struct {
    int seq_no;
    // what type here? more correct to allocate at run time once we know
    // of how many parts is the packet composed
    void chunks[MAX_CHUNKS];
} packet;


typedef struct packet packet;

void add_chunk(void *);

