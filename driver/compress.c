// see http://www.zlib.net/manual.html for more info

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../shared/structs.h"
#include "zlib.h"

#include "util.h"

#define LEVEL Z_BEST_COMPRESSION
#define COMPRESS 0
#define DECOMPRESS 1

static z_stream strm;

/** 
 * Initialize the stream to default values
 * Used in same way by compression and decompression
 * 
 * @param strm zstream to initialize
 */
void _reset_zstream(z_stream *strm) {
    strm->zalloc = Z_NULL;
    strm->zfree = Z_NULL;
    strm->opaque = Z_NULL;
}

/** 
 * Setup the stream for compression and decompression
 * 
 * @param strm 
 * @param data 
 * @param result 
 */
void _setup_zstream(z_stream *strm, const payload_t *data, payload_t *result) {
    // setup the correct values in the stream
    _reset_zstream(strm);
    strm->avail_in = data->len;
    strm->avail_out = result->len;
    strm->next_in = (unsigned char *) data->stream;
    strm->next_out = (unsigned char *) result->stream;
}

void printGained(streamlen_t before, streamlen_t after) {
    (void)before;
    (void)after;
    LOG_INFO("Compressed data is %.5f%% of original size.", ((float) after / before) * 100);
}

int _zlib_manage(int mode, const payload_t data, payload_t *result) {
    int ret;
    _setup_zstream(&strm, &data, result);
    switch (mode) {
    case COMPRESS:
        ret = deflateInit(&strm, LEVEL);
        break;
    case DECOMPRESS:
        ret = inflateInit(&strm);
    }
    assert(ret == Z_OK);
    
    // now check the difference between two approoaches
    return Z_OK;
}

int payloadCompress(const payload_t data, payload_t *result) {
    int ret;
    // maybe out should be bigger
    _setup_zstream(&strm, &data, result);
    streamlen_t tot_size = 0;

    /* allocate deflate state */
    // initialize the structures
    ret = deflateInit(&strm, LEVEL);
    // asserting it always works fine!
    assert(ret == Z_OK);
    // Forcing it to finish in one round only
    ret = deflate(&strm, Z_FINISH);    /* no bad return value */
    
    // strm.avail_out now contains how much data is still available to compress
    tot_size += (result->len - strm.avail_out);
    result->len = tot_size;
    assert(ret == Z_STREAM_END);

    // deallocate the structure
    deflateEnd(&strm);
    return Z_OK;
}

int payloadDecompress(const payload_t data, payload_t *result) {
    z_stream strm;
    int ret;
    streamlen_t tot_size = 0;
    _setup_zstream(&strm, &data, result);
    
    ret = inflateInit(&strm);
    assert(ret == Z_OK);

    ret = inflate(&strm, Z_FINISH);    /* no bad return value */
    tot_size += (result->len - strm.avail_out);
    result->len = tot_size;
    // the initialization was successful
    assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
    
    inflateEnd(&strm);
    return Z_OK;
}
