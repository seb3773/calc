#include "zx0em_runtime.h"
#include <stdio.h>

int main(void) {
    if (!zx0em_init()) {
        printf("Init failed\n");
        return 1;
    }
    printf("Init OK, %d assets\n", ZX0EM_ENTRY_COUNT);
    return 0;
}
