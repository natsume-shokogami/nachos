#include "syscall.h"

int main()
{
    char test[9] = "test.file";
    int result = Create(test);
    Halt();
}