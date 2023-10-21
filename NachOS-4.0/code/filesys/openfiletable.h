#ifndef OPENFILE_TABLE
#define OPENFILE_TABLE
//#include "openfile.h"
//#include "filesys.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>


#define FILE_DESCRIPTION_TABLE_SIZE 20

typedef int OpenFileId;
//class FileSystem;
//class OpenFile;

class FileTableEntry{
public:
    int permission; //0 for read-write, 1 for read-only, 2 for socket
    OpenFileId id;
    OpenFile *file;
    bool isOpen;
    char *fileName; //file name or id
    int position; //position if this is a file, port for socket
};

class OpenFileTable
{
private:
    FileSystem *_fs; //Assign root file system
    FileTableEntry* _fileDescriptionTable;
public:
    OpenFileTable();
    ~OpenFileTable();
    OpenFileTable(const OpenFileTable&);
    OpenFileTable& operator=(const OpenFileTable&);

    int Create(char *name);
    OpenFileId Open(char *name, int type);
    int Close(OpenFileId id);
    int Read(char *buffer, int size, OpenFileId id);
    int Write(char *buffer, int size, OpenFileId id);
    int Seek(int position, OpenFileId id);
    int Remove(char *name);
    int SocketTCP();
    int Connect(int socketid, char *ip, int port);
    int Send(int socketid, char *buffer, int len);
    int Receive(int socketid, char *buffer, int len);
};

#endif