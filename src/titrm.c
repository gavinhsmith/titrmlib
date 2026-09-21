#include "titrm.h"

#include <ti/screen.h>

void titrm_Init(void) {
    os_ClrHome();
}

void titrm_Print(const char *str) {
    os_PutStrFull(str);
}
