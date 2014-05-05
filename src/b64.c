#include "b64/cencode.h"
#include "b64/cdecode.h"

#include <string.h>

#include "b64.h"

// Encode a block of data in starta of length sz into a base64 string, stored
// in outbuf.
int b64encode(const char *starta, unsigned int sz, char *outbuf) {
  base64_encodestate s;
  base64_init_encodestate(&s);
  int cnt = base64_encode_block(starta, sz, outbuf, &s);
  cnt += base64_encode_blockend(outbuf + cnt, &s);
  return cnt;
}

// Decode a b64-encoded string into its data in outbuf.
int b64decode(const char *b64str, char *outbuf) {
  base64_decodestate s;
  base64_init_decodestate(&s);
  int cnt = base64_decode_block(b64str, strlen(b64str), outbuf, &s);
  outbuf[cnt] = 0;
  return cnt;
}

