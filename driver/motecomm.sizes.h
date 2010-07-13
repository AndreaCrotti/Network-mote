#ifndef _MOTECOMM_SIZES_H
#define _MOTECOMM_SIZES_H

/** util **/

typedef unsigned char stream_t;
typedef unsigned int streamlen_t;

typedef struct {
  stream_t const* stream;
  streamlen_t len;
} payload_t;

#define HD_BYTES_FROM_BITS(bits) (((bits)+7)/8)
#define MAX_PAYLOAD_SIZE(PROT) (1<<PROT##_HD_PAYLOAD)

/** MCP **/

#define MCP_HD_VERSION 4
#define MCP_HD_HEADER  4
#define MCP_HD_IDENT   8
#define MCP_HD_TYPE    8
#define MCP_HD_PORT    8
#define MCP_HD_PAYLOAD 8


#define MCP_HEADER_BITS (MCP_HD_VERSION+MCP_HD_HEADER+MCP_HD_IDENT+MCP_HD_TYPE+MCP_HD_PORT+MCP_HD_PAYLOAD)
#define MCP_HEADER_BYTES HD_BYTES_FROM_BITS(MCP_HEADER_BITS)

#define MCP_HD_VERSION_OFFSET 0
#define MCP_HD_HEADER_OFFSET  MCP_HD_VERSION_OFFSET+MCP_HD_VERSION
#define MCP_HD_IDENT_OFFSET   MCP_HD_HEADER_OFFSET+MCP_HD_HEADER
#define MCP_HD_TYPE_OFFSET    MCP_HD_IDENT_OFFSET+MCP_HD_IDENT
#define MCP_HD_PORT_OFFSET    MCP_HD_TYPE_OFFSET+MCP_HD_TYPE
#define MCP_HD_PAYLOAD_OFFSET MCP_HD_PORT_OFFSET+MCP_HD_PORT

typedef union {
  stream_t const* stream;
  struct {
#if NX_SWAP_NIBBLES
    unsigned header  :MCP_HD_HEADER;
    unsigned version :MCP_HD_VERSION;
#else
    unsigned version :MCP_HD_VERSION;
    unsigned header  :MCP_HD_HEADER;
#endif
    unsigned ident   :MCP_HD_IDENT;
    unsigned type    :MCP_HD_TYPE;
    unsigned port    :MCP_HD_PORT;
    unsigned payload :MCP_HD_PAYLOAD;
  }__attribute__((__packed__))* header;
} mcp_header_t;

/** MCCMP **/

#define MCCMP_HD_VERSION 4
#define MCCMP_HD_HEADER  4
#define MCCMP_HD_IDENT   8
#define MCCMP_HD_PROBLEM 8
#define MCCMP_HD_OFFSET  8
#define MCCMP_HD_PAYLOAD 8

#define MCCMP_HEADER_BITS (MCCMP_HD_VERSION+MCCMP_HD_HEADER+MCCMP_HD_IDENT+MCCMP_HD_PROBLEM+MCCMP_HD_OFFSET+MCCMP_HD_PAYLOAD)

#define MCCMP_HEADER_BYTES HD_BYTES_FROM_BITS(MCCMP_HEADER_BITS)

typedef union {
  stream_t const* stream;
  struct {
#if NX_SWAP_NIBBLES
    unsigned header  :MCCMP_HD_HEADER;
    unsigned version :MCCMP_HD_VERSION;
#else
    unsigned version :MCCMP_HD_VERSION;
    unsigned header  :MCCMP_HD_HEADER;
#endif
    unsigned ident   :MCCMP_HD_IDENT;
    unsigned problem :MCCMP_HD_PROBLEM;
    unsigned offset  :MCCMP_HD_OFFSET;
    unsigned payload :MCCMP_HD_PAYLOAD;
  } __attribute__((__packed__))* header;
} mccmp_header_t;


/** LAEP **/

#define LAEP_HD_VERSION 4
#define LAEP_HD_HEADER  4
#define LAEP_HD_IPV     4
#define LAEP_HD_TYPE    4
#define LAEP_HD_PAYLOAD 8

#define LAEP_HEADER_BITS (LAEP_HD_VERSION+LAEP_HD_HEADER+LAEP_HD_IPV+LAEP_HD_TYPE+LAEP_HD_PAYLOAD)

#define LAEP_HEADER_BYTES HD_BYTES_FROM_BITS(LAEP_HEADER_BITS)

typedef union {
  stream_t const* stream;
  struct {
#if NX_SWAP_NIBBLES
    unsigned header  :LAEP_HD_HEADER;
    unsigned version :LAEP_HD_VERSION;
    unsigned type    :LAEP_HD_TYPE;
    unsigned ipv     :LAEP_HD_IPV;
#else
    unsigned version :LAEP_HD_VERSION;
    unsigned header  :LAEP_HD_HEADER;
    unsigned ipv     :LAEP_HD_IPV;
    unsigned type    :LAEP_HD_TYPE;
#endif
    unsigned payload :LAEP_HD_PAYLOAD;
  } __attribute__((__packed__))* header;
} laep_header_t;

/** IFP **/

#define IFP_HD_VERSION  4
#define IFP_HD_HEADER   4
#define IFP_HD_PAYLOAD  8

#define IFP_HEADER_BITS (IFP_HD_VERSION+IFP_HD_HEADER+IFP_HD_PAYLOAD)

#define IFP_HEADER_BYTES HD_BYTES_FROM_BITS(IFP_HEADER_BITS)

typedef union {
  stream_t const* stream;
  struct {
#if NX_SWAP_NIBBLES
    unsigned header  :IFP_HD_HEADER;
    unsigned version :IFP_HD_VERSION;
#else
    unsigned version :IFP_HD_VERSION;
    unsigned header  :IFP_HD_HEADER;
#endif
    unsigned payload :IFP_HD_PAYLOAD;
  } __attribute__((__packed__))* header;
} ifp_header_t;

#endif
