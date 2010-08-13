#include "motecomm.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** args) {
  //
  mcp_header_t mht;
  mht.stream = (stream_t*)malloc(100);
  mht.header->version = 1;
  mht.header->header = 5;
  mht.header->ident = 0xDE;
  mht.header->type = 0xAD;
  mht.header->port = 0xBE;
  mht.header->payload = 0xEF;
  int i = 0;
  printf("If it works, you should see '15 DE AD BE EF'\n");
  printf("NX_SWAP_NIBBLES: %u\n",(unsigned)NX_SWAP_NIBBLES);
  for (; i < MCP_HEADER_BYTES; i++) {
    printf("%02X ",(unsigned)(mht.stream[i]));
  }
  return 0;
  assert(argc >= 3);
  mcp_t* mcp = open_mcp_connection(args[1],args[2],NULL);
  if (!mcp) {
    printf("There was an error opening the connection to %s over device %s.",args[2],args[1]);
    return 1; 
  }
  assert(mcp);
  assert(mcp->get_comm);
  assert(mcp->get_comm(mcp));
  assert(mcp->get_comm(mcp)->read);
  mccmp(NULL,mcp);
  laep(NULL,mcp);
  ifp(NULL,mcp);
  while (1) {
    mcp->get_comm(mcp)->read(mcp->get_comm(mcp));
    printf("there was a message\n");
  }
  return 0;
}
