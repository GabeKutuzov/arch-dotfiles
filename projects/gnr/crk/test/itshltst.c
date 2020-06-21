/* Test routine for itershl */

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "itershl.h"

#define RAD1  2.0
#define RAD2  3.0

void main() {

    IterShl IShl;

    InitShellIndexing(&IShl, 0.5, 1.0, 1.0, 11.0, 12.0, 13.0,
       RAD1, RAD2, 100, 100, 14);

    while (GetNextPointInShell(&IShl)) {
       printf(" Returned point at %d, %d, %d\n", IShl.ix, IShl.iy, IShl.iz);
       }
    }
