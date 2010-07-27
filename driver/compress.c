// see http://www.zlib.net/manual.html for more info

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../shared/structs.h"
#include "zlib.h"

/* see if the dimension is important or not */
#define LEVEL Z_BEST_COMPRESSION

// TODO: see how much data has the last one
// compress original data in the given result
int payload_compress(payload_t data, payload_t *result) {
    int ret;
    z_stream strm;
    // maybe out should be bigger
    unsigned char *in = (unsigned char *) data.stream;
    unsigned char *out = (unsigned char *) result->stream;
    streamlen_t *tot_size = &(result->len);
    *tot_size = 0;

    /* allocate deflate state */
    // initialize the structures
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, LEVEL);
    // if initialized correctly
    if (ret != Z_OK)
        return ret;

    strm.avail_in = data.len;
    strm.avail_out = data.len;
    printf("now ret in and out = %d, %d, %d\n", ret, strm.avail_in, strm.avail_out);
    
    // is this thing enough for it?
    strm.next_in = in;
    strm.next_out = out;
    ret = deflate(&strm, Z_FINISH);    /* no bad return value */
    
    *tot_size += (data.len - strm.avail_out);

    if (ret == Z_OK) {
        // we get twice the data
        strm.avail_out = data.len;
        ret = deflate(&strm, Z_FINISH);
        *tot_size += (data.len - strm.avail_out);
    }

    printf("now ret in and out = %d, %d, %d\n", ret, strm.avail_in, strm.avail_out);
    printf("tot_size needed %d\n", *tot_size);
    assert(ret == Z_STREAM_END);
    deflateEnd(&strm);
    return Z_OK;
}

int payload_decompress(payload_t data, payload_t *result) {
    z_stream strm;
    int ret;
    // maybe out should be bigger
    unsigned char *in = (unsigned char *) data.stream;
    unsigned char *out = (unsigned char *) result->stream;
    streamlen_t *tot_size = &(result->len);

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = inflateInit(&strm);
    
    if (ret != Z_OK)
        return ret;

    strm.avail_in = data.len;
    strm.avail_out = data.len;
    strm.next_in = in;
    strm.next_out = out;
    ret = inflate(&strm, Z_FINISH);    /* no bad return value */
    *tot_size = data.len - strm.avail_out;
    printf("now ret in and out = %d, %d, %d\n", ret, strm.avail_in, strm.avail_out);
    // the initialization was successful
    assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
    
    inflateEnd(&strm);
    return Z_OK;
}
