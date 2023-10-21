
#include "syscall.h"

int main()
{
    char* firstFile = "./cat_test_short.txt";
    char* resultFile = "./res.txt";
    char* failure = "Copy failed\n";
    char* writeFailure = "Write failed\n";
    int f = Open(firstFile, 0);
    if (f == -1)
    {
        Write(failure, 12, 1);
        Halt();
    }
    else
    {
        char t[1025];
        int r, r2;
        //Create(resultFile);
        int f2;
        f2 = Open(resultFile, 0);
        if (f2 == -1)
        {
            Write(failure, 12, 1);
            Halt();
        }
        r = Read(t, 1024, f);

        while (r > 0)
        {
            r2 = Write(t, 1024, f2);
            if (r2 == -1)
            {
		Write(writeFailure, 13, 1);
                break;
            }
            r = Read(t, 1024, f);
        }
        r = Close(f); r2 = Close(f2);
        Halt();
    }
    Halt();
    return 0;

}
