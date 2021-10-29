#include <string.h>
#include "computer.h"

/*
    根据《计算机系统概论》所做的模拟计算机
 */

int main() {
    WORD program[] = {0x0004, 0x0000, 0x0042, 0x0043,
                      0x21FC, 0x31FC, 0x21FB, 0xF025};
    memcpy(mem, program, sizeof(program));
    run();
    return 0;
}