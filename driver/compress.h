#ifndef COMPRESS_H
#define COMPRESS_H

int payloadCompress(payload_t data, payload_t *result);
int payloadDecompress(payload_t data, payload_t *result);
void printGained(streamlen_t before, streamlen_t after);

#endif /* COMPRESS_H */
