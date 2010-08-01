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
 */
void _setup_zstream(z_stream *strm, const payload_t *data, payload_t *result) {
    // setup the correct values in the stream
    _reset_zstream(strm);
    strm->avail_in = data->len;
    strm->avail_out = result->len;
    strm->next_in = (unsigned char *) data->stream;
    strm->next_out = (unsigned char *) result->stream;
}

void print_gained(streamlen_t before, streamlen_t after) {
    LOG_INFO("Compressed data is %.5f%% of original size.", ((float) after / before) * 100);
}

int _zlib_manage(int mode, const payload_t data, payload_t *result) {
    int ret;
    _setup_zstream(&strm, &data, result);
    switch (mode) {
    case COMPRESS:
        deflateInit(&strm, LEVEL);
        ret = deflate(&strm, Z_FINISH);
        break;
    case DECOMPRESS:
        inflateInit(&strm);
        ret = inflate(&strm, Z_FINISH);
        break;
    }
    assert(ret == Z_STREAM_END);

    result->len = (result->len - strm.avail_out);
    return Z_OK;
}

int payload_compress(const payload_t data, payload_t *result) {
    return _zlib_manage(COMPRESS, data, result);
}

int payload_decompress(const payload_t data, payload_t *result) {
    return _zlib_manage(DECOMPRESS, data, result);
}
