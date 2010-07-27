// see http://www.zlib.net/manual.html for more info

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../shared/structs.h"
#include "zlib.h"

/* see if the dimension is important or not */
#define LEVEL Z_BEST_COMPRESSION

void init_zstream(z_stream *strm) {
    strm->zalloc = Z_NULL;
    strm->zfree = Z_NULL;
    strm->opaque = Z_NULL;
}

void printGained(streamlen_t before, streamlen_t after) {
    printf("gained %.5f\n", ((float) after / before) * 100);
}

// TODO: see how much data has the last one
// compress original data in the given result
int payloadCompress(payload_t data, payload_t *result) {
    int ret;
    z_stream strm;
    // maybe out should be bigger
    strm.avail_in = data.len;
    strm.avail_out = result->len;
    strm.next_in = (unsigned char *) data.stream;
    strm.next_out = (unsigned char *) result->stream;
    streamlen_t tot_size = 0;

    init_zstream(&strm);
    /* allocate deflate state */
    // initialize the structures
    ret = deflateInit(&strm, LEVEL);
    // if initialized correctly
    if (ret != Z_OK)
        return ret;

    ret = deflate(&strm, Z_FINISH);    /* no bad return value */
    
    tot_size += (result->len - strm.avail_out);
    result->len = tot_size;
    assert(ret == Z_STREAM_END);
    deflateEnd(&strm);
    return Z_OK;
}

int payloadDecompress(payload_t data, payload_t *result) {
    z_stream strm;
    int ret;
    streamlen_t tot_size = 0;
    strm.avail_in = data.len;
    strm.avail_out = result->len;
    strm.next_in = (unsigned char *) data.stream;
    strm.next_out = (unsigned char *) result->stream;
    
    init_zstream(&strm);
    ret = inflateInit(&strm);
    
    if (ret != Z_OK)
        return ret;

    ret = inflate(&strm, Z_FINISH);    /* no bad return value */
    tot_size += (result->len - strm.avail_out);
    result->len = tot_size;

    printf("now ret in and out = %d, %d, %d\n", ret, strm.avail_in, strm.avail_out);
    // the initialization was successful
    assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
    
    inflateEnd(&strm);
    return Z_OK;
}
