#include "syscall.h"

#define ConsoleInput 0
#define ConsoleOutput 1

//int Strlen(char* str);

int main()
{
    //OpenFileId input = ConsoleInput;
    //OpenFileId output = ConsoleOutput;

    char *openNotify = "Input file name: ";
    char *error = "Error opening file";
    char *noFileSpecified = "No file specified";

    Write(openNotify, 17, 1);

    int j;
    j = 0;

    char fileName[33];
    char ch;

    do {
        Write(ch, 1, 0);
        fileName[j] =  &ch;
        j++;
    }while (j < 33-1 && ch != '\n');
    fileName[j] = '\0';
    

    int f;
    f = Open(fileName, 0);
    
    if (f == -1)
    {
        Write(error, 18, 1);
        Halt();
    }
    
    char buffer[1025];
    int result;
    while (1)
    {   
        result = -1;
        result = Read(buffer, 1024, f);
        if (result == 0)
        {
            break;
        }
        Write(buffer, result, 1);
    }
    Halt();
    return 0;
}
