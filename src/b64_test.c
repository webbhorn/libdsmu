#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mem.h"
#include "b64.h"

int main(void) {

    const char *testString = "aAbBcCdDeEfFgGhHiIjJ!";
    char buf[30] = {0};

    b64encode(testString, 21, buf);
    
    char outbuf[10] = {0};
    
    b64decode(buf, outbuf);

    printf("%s\n", outbuf);

    return 0;
}
  
