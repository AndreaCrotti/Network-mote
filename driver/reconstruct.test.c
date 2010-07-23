int num_packets = 10;
void test_addressing();
void test_last();

// doing some simple testing
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
/* FIXME FIXME FIXME
    // give it a real function
    initReconstruction();
    ipv6Packet *pkt = calloc(num_packets, sizeof(ipv6Packet));
    
    // create some fake payload to create correctly the payload
    
    test_last();
    stream_t x[2] = {0, 1};

    for (int i = 0; i < num_packets; i++) {
        make_ipv6_packet(&(pkt[i]), 0, i, num_packets, x, 2);
        addChunk((void *) &pkt[i]);
    }

    get_packet(0);
    // 0 for example can't be found
    // assert(get_packet(0) == NULL); 

    test_addressing();

    // TODO: add assertions for correct tot_size and more

    free(pkt);
*/
    return 0;
}
/* FIXME
// at every position there should be something such that
// ( POS % seq_no) == 0
void test_addressing() {
    for (int i = 0; i < MAX_RECONSTRUCTABLE; i++) {
        packet_t *pkt = &(temp_packets[i]);
        assert((pkt->seq_no % MAX_RECONSTRUCTABLE) == i);
   }
}

void test_last() {
    ipv6Packet *pkt = malloc(sizeof(ipv6Packet));
    stream_t *payload = malloc(0);
    make_ipv6_packet(pkt, 0, 0, 1, payload, 0);
    assert(is_last(pkt));
    free(pkt);
}

*/
