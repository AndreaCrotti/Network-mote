#ifndef _MOTECOMM_SIZES_H
#define _MOTECOMM_SIZES_H

/** util **/
#define HD_BYTES_FROM_BITS(bits) (((bits)+7)/8)

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

/** MCCMP **/

#define MCCMP_HD_VERSION 4
#define MCCMP_HD_HEADER  4
#define MCCMP_HD_IDENT   8
#define MCCMP_HD_PROBLEM 8
#define MCCMP_HD_OFFSET  8
#define MCCMP_HD_PAYLOAD 8

#define MCCMP_HEADER_BITS (MCCMP_HD_VERSION+MCCMP_HD_HEADER+MCCMP_HD_IDENT+MCCMP_HD_PROBLEM+MCCMP_HD_OFFSET+MCCMP_HD_PAYLOAD)

#define MCCMP_HEADER_BYTES HD_BYTES_FROM_BITS(MCCMP_HEADER_BITS)

/** LAEP **/

#define LAEP_HD_VERSION 4
#define LAEP_HD_HEADER  4
#define LAEP_HD_IPV     4
#define LAEP_HD_TYPE    4
#define LAEP_HD_PAYLOAD 8

#define LAEP_HEADER_BITS (LAEP_HD_VERSION+LAEP_HD_HEADER+LAEP_HD_IPV+LAEP_HD_TYPE+LAEP_HD_PAYLOAD)

#define LAEP_HEADER_BYTES HD_BYTES_FROM_BITS(LAEP_HEADER_BITS)

/** IFP **/

#define IFP_HD_VERSION  4
#define IFP_HD_HEADER   4
#define IFP_HD_PAYLOAD  8

#define IFP_HEADER_BITS (IFP_HD_VERSION+IFP_HD_HEADER+IFP_HD_PAYLOAD)

#define IFP_HEADER_BYTES HD_BYTES_FROM_BITS(IFP_HEADER_BITS)

#endif
