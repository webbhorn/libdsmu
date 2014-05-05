#ifndef _B64_H_
#define _B64_H_

int b64encode(const char *starta, unsigned int sz, char *outbuf);
int b64decode(const char *b64str, char *outbuf);

#endif  // _B64_H_
