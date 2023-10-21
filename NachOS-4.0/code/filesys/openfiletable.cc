#include "openfile.h"
#include "filesys.h"
#include "directory.h"
#include "sysdep.h"
#include "openfiletable.h"
#include "string.h"
#include "synchconsole.h"
#include "debug.h"
#include <cstring>
#include <errno.h>

OpenFileTable::OpenFileTable()
{
    this->_fs = kernel->fileSystem;
    this->_fileDescriptionTable = new FileTableEntry[FILE_DESCRIPTION_TABLE_SIZE];
    for (int i = 0; i < FILE_DESCRIPTION_TABLE_SIZE; i++)
    {
        this->_fileDescriptionTable[i].file = NULL;
        this->_fileDescriptionTable[i].isOpen = false;
        this->_fileDescriptionTable[i].permission = 0;
        this->_fileDescriptionTable[i].fileName = NULL;
        this->_fileDescriptionTable[i].position = 0;
        this->_fileDescriptionTable[i].id = -1;
    }
}

OpenFileTable::~OpenFileTable()
{   
    for (int i = 0; i < FILE_DESCRIPTION_TABLE_SIZE; i++)
    {
        delete[] _fileDescriptionTable[i].fileName;
    }
    delete[] this->_fileDescriptionTable;
}

OpenFileTable::OpenFileTable(const OpenFileTable& arg)
{
    this->_fs = arg._fs;
    this->_fileDescriptionTable = new FileTableEntry[FILE_DESCRIPTION_TABLE_SIZE];
    for (int i = 0; i < FILE_DESCRIPTION_TABLE_SIZE; i++)
    {
        this->_fileDescriptionTable[i].file = arg._fileDescriptionTable[i].file;
        this->_fileDescriptionTable[i].isOpen = arg._fileDescriptionTable[i].isOpen;
        this->_fileDescriptionTable[i].permission = arg._fileDescriptionTable[i].permission;
        this->_fileDescriptionTable[i].fileName = new char[FileNameMaxLen+1];
        strncpy(this->_fileDescriptionTable[i].fileName, arg._fileDescriptionTable[i].fileName, FileNameMaxLen);
        this->_fileDescriptionTable[i].fileName[FileNameMaxLen] = '\0';
        this->_fileDescriptionTable[i].position = arg._fileDescriptionTable[i].position;
    }
}

OpenFileTable& OpenFileTable::operator=(const OpenFileTable& arg)
{
    this->_fs = arg._fs;
    for (int i = 0; i < FILE_DESCRIPTION_TABLE_SIZE; i++)
    {
        delete[] _fileDescriptionTable[i].fileName;
    }
    delete[] this->_fileDescriptionTable;
    this->_fileDescriptionTable = new FileTableEntry[FILE_DESCRIPTION_TABLE_SIZE];
    for (int i = 0; i < FILE_DESCRIPTION_TABLE_SIZE; i++)
    {
        this->_fileDescriptionTable[i].file = arg._fileDescriptionTable[i].file;
        this->_fileDescriptionTable[i].isOpen = arg._fileDescriptionTable[i].isOpen;
        this->_fileDescriptionTable[i].permission = arg._fileDescriptionTable[i].permission;
        this->_fileDescriptionTable[i].fileName = new char[FileNameMaxLen+1];
        strncpy(this->_fileDescriptionTable[i].fileName, arg._fileDescriptionTable[i].fileName, FileNameMaxLen);
        this->_fileDescriptionTable[i].fileName[FileNameMaxLen] = '\0';
        this->_fileDescriptionTable[i].position = arg._fileDescriptionTable[i].position;
    }
    return *this;
}


int OpenFileTable::Create(char *name)
{
    char *temp = new char[FileNameMaxLen + 1];
    strncpy(temp, name, FileNameMaxLen);
    temp[FileNameMaxLen] = '\0';

    bool result = this->_fs->Create(temp);
    if (result)
    {
        delete[] temp;
        return 0;
    }
    delete[] temp;
    return -1;
}

OpenFileId OpenFileTable::Open(char *name, int type)
{
    DEBUG(dbgFile, "\nFile type of file name " << name << " is " << type);
    if (type != 1 && type != 0)
    {
        return -1;
    }
    ASSERT(type == 1 || type == 0);

    OpenFile *openedFile = this->_fs->Open(name);
    DEBUG(dbgFile, "\nAccess of open file: " << openedFile);

    if (openedFile == NULL)
    {
        DEBUG(dbgFile, "\nReturn value" << -1 );
        //Error when searching file or file name not found
        return -1;
    }

    for (int i = 2; i < FILE_DESCRIPTION_TABLE_SIZE; i++)
    {
        if (this->_fileDescriptionTable[i].file == NULL )
        {
            this->_fileDescriptionTable[i].file = openedFile;
            this->_fileDescriptionTable[i].isOpen = true;
            this->_fileDescriptionTable[i].permission = type;
            this->_fileDescriptionTable[i].fileName =  new char[FileNameMaxLen + 1];
            strncpy(_fileDescriptionTable[i].fileName, name, FileNameMaxLen);
            this->_fileDescriptionTable[i].fileName[FileNameMaxLen] = '\0';
            this->_fileDescriptionTable[i].position = 0;
            ASSERT(this->_fileDescriptionTable[i].file != NULL && this->_fileDescriptionTable[i].fileName != NULL);
            ASSERT(i < FILE_DESCRIPTION_TABLE_SIZE && i > 1);
            DEBUG(dbgFile, "\nReturn value" << i );
            return i;
        }
        DEBUG(dbgFile, "\n File at index " << i << ": " << this->_fileDescriptionTable[i].file );
    }
    return -1;
}

int OpenFileTable::Close(OpenFileId id)
{
    if (id >= FILE_DESCRIPTION_TABLE_SIZE || id <= 1)
    {
        return -1;
    }
    if (this->_fileDescriptionTable[id].isOpen == true)
    {
        this->_fileDescriptionTable[id].isOpen = false;  
    }
    if (this->_fileDescriptionTable[id].permission == 1 
    || this->_fileDescriptionTable[id].permission == 0)
    {
        this->_fileDescriptionTable[id].file = NULL;
        delete[] this->_fileDescriptionTable[id].fileName;
        this->_fileDescriptionTable[id].fileName = NULL;
        this->_fileDescriptionTable[id].position = 0;
        this->_fileDescriptionTable[id].id = -1;
        DEBUG(dbgFile, "\nReturn value" << 0 );
        return 0;
    }
    else
    {
        //Socket file
        int closeSocketResult = close(this->_fileDescriptionTable[id].id);
        this->_fileDescriptionTable[id].file = NULL;
        delete[] this->_fileDescriptionTable[id].fileName;
        this->_fileDescriptionTable[id].fileName = NULL;
        this->_fileDescriptionTable[id].position = -1;
        return closeSocketResult;
    }
    
    return 0;
}

int OpenFileTable::Read(char *buffer, int size, OpenFileId id)
{
    if (id >= FILE_DESCRIPTION_TABLE_SIZE || id == 1)
    {
        return -1;
    }
    if (id == 0)
    {
        int readConsoleBufferSize = 0;
        buffer = new char[size + 1];

        DEBUG(dbgFile, "Reading char from console..., buffer: \n");
        char oneChar;
        do{
            oneChar = kernel->synchConsoleIn->GetChar();
            readConsoleBufferSize++;
            DEBUG(dbgFile, oneChar);
        }while (oneChar != 0 && readConsoleBufferSize < size);
        buffer[readConsoleBufferSize] = '\0';
        DEBUG(dbgFile, "\n");
        DEBUG(dbgFile, "Number of chars read: " << readConsoleBufferSize);
        return readConsoleBufferSize;
    }
    if (this->_fileDescriptionTable[id].file == NULL)
    {
        return -1;
    }

    DEBUG(dbgFile, "Current file's position: " <<this->_fileDescriptionTable[id].position + 1 <<
    "Current file's length" << this->_fileDescriptionTable[id].file->Length() << "\n");
    if (this->_fileDescriptionTable[id].position + 1 >= this->_fileDescriptionTable[id].file->Length())
    {
        return -1;
    }

    ASSERT(this->_fileDescriptionTable[id].position + 1 < this->_fileDescriptionTable[id].file->Length());

    int readBufferSize = this->_fileDescriptionTable[id].file->ReadAt(buffer, size, this->_fileDescriptionTable[id].position);
    DEBUG(dbgFile, "Read chars from files..., buffer size" << size <<", position: " << 
    this->_fileDescriptionTable[id].position << ", actual chars read: " << readBufferSize << "buffer: \n");

    DEBUG(dbgFile, buffer << "\n");
    this->_fileDescriptionTable[id].position += readBufferSize;
    DEBUG(dbgFile, "Position now: " << _fileDescriptionTable[id].position << "\n");
    return readBufferSize;
}
int OpenFileTable::Write(char *buffer, int size, OpenFileId id)
{
    DEBUG(dbgFile, "Start writing to file with id " << id << " with " << size << "chars \n");
    if (id >= FILE_DESCRIPTION_TABLE_SIZE || id == 0)
    {
        return -1;
    }
    if (id == 1)
    {
        int writeConsoleBufferSize = 0;
        
        DEBUG(dbgSys, "Writing char to console, buffer: \n");
        char oneChar;
        do{
            oneChar = buffer[writeConsoleBufferSize];
            kernel->synchConsoleOut->PutChar(oneChar);
            writeConsoleBufferSize++;
            DEBUG(dbgFile, oneChar);
        }while (oneChar != 0 && writeConsoleBufferSize < size);
        DEBUG(dbgFile, "\n");
        DEBUG(dbgFile, "Number of chars read: " << writeConsoleBufferSize);
        return writeConsoleBufferSize;
    }
    if (this->_fileDescriptionTable[id].file == NULL)
    {
        return -1;
    }
    
    DEBUG(dbgFile, "Current file's position: " <<this->_fileDescriptionTable[id].position + 1 <<
    "Current file's length" << this->_fileDescriptionTable[id].file->Length() << "\n");
    
    int writeBufferSize = this->_fileDescriptionTable[id].file->WriteAt(buffer, size, this->_fileDescriptionTable[id].position);
    DEBUG(dbgFile, "Write chars from files..., buffer size" << size <<", position: " << 
    this->_fileDescriptionTable[id].position << ", actual chars read: " << writeBufferSize << "buffer: \n");
    
    DEBUG(dbgFile, buffer << "\n");
    this->_fileDescriptionTable[id].position += writeBufferSize;
    return writeBufferSize;
}

int OpenFileTable::Seek(int position, OpenFileId id)
{
    if (id >= FILE_DESCRIPTION_TABLE_SIZE || id <= 1)
    {
        //If index out of range or seeking in console read/write file
        return -1;
    }
    if (this->_fileDescriptionTable[id].file == NULL)
    {
        return -1;
    }
    if (this->_fileDescriptionTable[id].permission != 2)
    {
        int moveTo = position;
        if (position == -1)
        {
            //If position equals -1, move to the end of the file
            moveTo = this->_fileDescriptionTable[id].file->Length();
        }
        if (moveTo >= this->_fileDescriptionTable[id].file->Length())
        {
            DEBUG(dbgFile, "Position is out of range (position > file's length).\n");
            return -1;
        }
        this->_fileDescriptionTable[id].position = moveTo;
        ASSERT(moveTo < this->_fileDescriptionTable[id].file->Length());
        DEBUG(dbgFile, "Move file cursor to position " << moveTo << "for file with file id: " << id <<".\n");
        return 0;
    }
    else{
        DEBUG(dbgNet, "Network files cannot be seeked.\n");
        return -1;
    }
    return -1;
    
}
int OpenFileTable::Remove(char *name)
{
    char *nameWithMaxFileNameSize = new char[FileNameMaxLen + 1];
    strncpy(nameWithMaxFileNameSize, name, FileNameMaxLen);
    nameWithMaxFileNameSize[FileNameMaxLen] = '\0';
    for (int i = 2; i < FILE_DESCRIPTION_TABLE_SIZE; i++)
    {
        //if there's a file with that name open
        if (strcmp(name, this->_fileDescriptionTable[i].fileName) == 0)
        {
            delete[] nameWithMaxFileNameSize;
            return -1;
        }
        
    }
    bool result = this->_fs->Remove(name);
    delete[] nameWithMaxFileNameSize;
    if (result == true)
    {
        return 0;
    }
    return -1;
}

int OpenFileTable::SocketTCP()
{
    DEBUG(dbgNet, "Initiate a new socket\n");


    for (int i = 2; i < FILE_DESCRIPTION_TABLE_SIZE; i++)
    {
        if (this->_fileDescriptionTable[i].fileName == NULL)
        {
            this->_fileDescriptionTable[i].file = NULL;
            this->_fileDescriptionTable[i].isOpen = true;
            this->_fileDescriptionTable[i].permission = 2;
            this->_fileDescriptionTable[i].fileName = NULL;
            this->_fileDescriptionTable[i].position = 0;

            ASSERT(i < FILE_DESCRIPTION_TABLE_SIZE && i > 1);
            DEBUG(dbgFile, "\nReturn value" << i );
            DEBUG(dbgFile, "\n" << -1 );
            int socketId = socket(AF_INET, SOCK_STREAM, 0);
            if (socketId != -1)
            {
                this->_fileDescriptionTable[i].id = socketId;
            }
            else
            {
                return -1;
            }
            return i;
        }
        DEBUG(dbgFile, "\n File at index " << i << ": " << this->_fileDescriptionTable[i].file );
    }
    return -1;
}

int OpenFileTable::Connect(int socketId, char *ip, int port)
{
    DEBUG(dbgNet, "Connect to virtual socket ID: " << socketId << ", IP: " << *ip << ", Port: " << port << "\n");
    sockaddr_in server;
    if (socketId < 2 || socketId >= FILE_DESCRIPTION_TABLE_SIZE)
    {
        DEBUG(dbgNet, "File table index out of range.\n");
        return -1;
    }
    int socket = this->_fileDescriptionTable[socketId].id;
    DEBUG(dbgNet, "Socket ID: " << socketId << "\n");

    if (socket < 0)
    {
        DEBUG(dbgNet, "Invalid socket ID.\n");
        return -1;
    }
    server.sin_family = AF_INET;
    server.sin_port = port;
    in_addr_t address = inet_addr(ip);
    DEBUG(dbgNet, "IP Address: " << address << "\n");
    if (address == (in_addr_t)(-1))
    {
        DEBUG(dbgNet, "Invalid IP address.\n");
        return -1;
    }
    server.sin_addr.s_addr = address;
    int len = sizeof(server);

    if (connect(socket, (struct sockaddr *)&server, len) < 0)
    {
        DEBUG(dbgNet, "Cannot connect to server");
        return -1;
    }
    else
    {
        DEBUG(dbgNet, "Successfully connect to server.\n");
        return 0;
    }
    ASSERTNOTREACHED();
}

int OpenFileTable::Send(int socketId, char *buffer, int len)
{
    DEBUG(dbgNet, "Send to virtual socket ID: " << socketId << ", buffer: " << buffer << ", len: " << len << "\n");
    int socket = this->_fileDescriptionTable[socketId].id;
    if (socket < 0)
    {
        DEBUG(dbgNet, "Invalid socket ID.\n");
        return -1;
    }
    if (len < 0)
    {
        DEBUG(dbgNet, "Invalid string length.\n");
    }

    if (buffer == NULL)
    {
        return -1;
    }
    DEBUG(dbgNet, "Send buffer: " << *buffer << "\n");

    int length = len;
    if (len != strlen(buffer))
    {
        length = strlen(buffer);
        DEBUG(dbgNet, "Warning: len and buffer's real length are mismatch.\n");
    }

    int result = send(socket, buffer, length, 0);

    if (errno == 32)
    {
        DEBUG(dbgNet, "No connection detected.\n");
        return -1;
    }

    if (result < 0)
    {
        DEBUG(dbgNet, "Sending buffer: " << *buffer << " failed.\n");
        return -1;
    }
    else
    {
        return result;
    }
    ASSERTNOTREACHED();
}

int OpenFileTable::Receive(int socketId, char *buffer, int len)
{
    DEBUG(dbgNet, "Send to virtual socket ID: " << socketId << ", buffer: " << buffer << ", len: " << len << "\n");
    int socket = this->_fileDescriptionTable[socketId].id;
    if (socket < 0)
    {
        DEBUG(dbgNet, "Invalid socket ID.\n");
        return -1;
    }
    if (len < 0)
    {
        DEBUG(dbgNet, "Invalid string length.\n");
    }

    if (buffer == NULL)
    {
        DEBUG(dbgNet, "Null return char array.\n");
        return -1;
    }
    int length = len;
    if ((unsigned int)len != strlen(buffer))
    {
        length = strlen(buffer);
        DEBUG(dbgNet, "Warning: len and buffer's real length are mismatch.\n");
    }

    int result = recv(socket, buffer, length, 0);
    DEBUG(dbgNet, "Buffer received: " << *buffer << "\n");

    if (errno == 32)
    {
        DEBUG(dbgNet, "No connection detected.\n");
        return -1;
    }

    if (result < 0)
    {
        DEBUG(dbgNet, "Receving buffer failed.\n");
        return -1;
    }
    else
    {
        DEBUG(dbgNet, "Receiving buffer: " << buffer << " successfully.\n");
        return result;
    }
    ASSERTNOTREACHED();
}