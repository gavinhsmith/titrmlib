#include <ti/getcsc.h>

#include "titrm.h"

int main(void) {
    titrm_Init();
    titrm_Print("titrmlib: basic test");

    while (!os_GetCSC());

    return 0;
}
