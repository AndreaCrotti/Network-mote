#ifndef COMPRESS_H
#define COMPRESS_H

/** 
 * Compress the payload given into the result
 * 
 * @param data payload to compress
 * @param result where to write data
 * 
 * @return return code
 */
int payload_compress(const payload_t data, payload_t *result);

/** 
 * Decompress the data
 * 
 * @param data 
 * @param result 
 * 
 * @return 
 */
int payload_decompress(const payload_t data, payload_t *result);

// some statistics
void print_gained(streamlen_t before, streamlen_t after);

void init_compression(void);

void close_compression(void);

#endif /* COMPRESS_H */
