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
int payloadCompress(const payload_t data, payload_t *result);

/** 
 * Decompress the data
 * 
 * @param data 
 * @param result 
 * 
 * @return 
 */
int payloadDecompress(const payload_t data, payload_t *result);

// some statistics
void printGained(streamlen_t before, streamlen_t after);

#endif /* COMPRESS_H */
