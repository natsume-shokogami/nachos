// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"
#include "openfiletable.h"
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	is in machine.h.
//----------------------------------------------------------------------

#define FileNameMaxLen 32

char* UserToSystem(int virtualAddress, int limit)
{
	int i;
	int oneChar;
	char *kernelBuffer = new char[limit+1];

	if (kernelBuffer == NULL)
	{
		return kernelBuffer;
	}

	memset(kernelBuffer, 0, 1);
	//printf("\n File name u2s:");

	for (i = 0; i < limit; i++)
	{
		kernel->machine->ReadMem(virtualAddress+i, 1, &oneChar);
		kernelBuffer[i] = (char)oneChar;
		if (oneChar == 0)
		{
			//printf("%c", kernelBuffer[i]);
			break;
		}
	}
	return kernelBuffer;
}

int SystemToUser(int virtualAddress, int len, char* buffer)
{
	if (len < 0) { return -1; }
	if (len == 0) { return len; }
	int i = 0; 
	int oneChar;
	do {
		oneChar = (int)buffer[i];
		kernel->machine->WriteMem(virtualAddress+i, 1, oneChar);
		i++;
	}while (i < len && oneChar != 0);

	return i;
}

void incrementProgramCounter()
{
	kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));
	/* set program counter to next instruction (all Instructions are 4 byte wide)*/
	kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);
	/* set next program counter for brach execution */
	kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
}

void
ExceptionHandler(ExceptionType which)
{
    int type = kernel->machine->ReadRegister(2);

    DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");

    switch (which) {
    case SyscallException:
      switch(type) {
      case SC_Halt:
	  {
		DEBUG(dbgSys, "Shutdown, initiated by user program.\n");

		SysHalt();

		ASSERTNOTREACHED();
		break;
	  }
	  case SC_Exit:
	  {
		//Temp 
		DEBUG(dbgSys, "[SystemCall] Process named " << kernel->currentThread->getName() << " has invoked syscall Exit\n");
		SysHalt();

		ASSERTNOTREACHED();
		break;
	  }
      case SC_Add:
	  {
		DEBUG(dbgSys, "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");
		
		/* Process SysAdd Systemcall*/
		int result;
		result = SysAdd(/* int op1 */(int)kernel->machine->ReadRegister(4),
				/* int op2 */(int)kernel->machine->ReadRegister(5));

		DEBUG(dbgSys, "Add returning with " << result << "\n");
		/* Prepare Result */
		
		kernel->machine->WriteRegister(2, (int)result);
		
		/* Modify return point */
		incrementProgramCounter();
		return;
		
		ASSERTNOTREACHED();
		break;
	  }
	  case SC_Create:
	  {
	
		int virtualAddr = kernel->machine->ReadRegister(4);
		char *fileName = UserToSystem(virtualAddr, FileNameMaxLen+1);
		DEBUG(dbgFile, "Creating files:" << fileName << "\n");

		if (fileName == NULL)
		{
			DEBUG(dbgFile, "Not enough memory on system to create file.\n");
			kernel->machine->WriteRegister(2, -1);
			incrementProgramCounter();
			return;
		}
		DEBUG(dbgFile, "Finished reading file name: " << fileName <<"\n");
		int result;
		result = kernel->openingFiles->Create(fileName);
		delete[] fileName;

		kernel->machine->WriteRegister(2, result);
		incrementProgramCounter();
		return;

		ASSERTNOTREACHED()
		break;
		}

	case SC_Open:
	{
		int virtualAddr = kernel->machine->ReadRegister(4);
		char *fileName = UserToSystem(virtualAddr, FileNameMaxLen+1);
		DEBUG(dbgFile, "Opening files:" << fileName << "; Type: ");

		if ((int)kernel->machine->ReadRegister(5) == 0)
		{
			DEBUG(dbgFile, "Read-write\n");
		}
		else if ((int)kernel->machine->ReadRegister(5) == 1)
		{
			DEBUG(dbgFile, "Read-only\n");
		}
		else
		{
			DEBUG(dbgFile, "Invalid/Unimplemented permission.\n");
		}

		if (fileName == NULL)
		{
			DEBUG(dbgFile, "Not enough memory on system to create file.\n");
			kernel->machine->WriteRegister(2, -1);
			incrementProgramCounter();
			return;
		}
		DEBUG(dbgFile, "Finished reading file name: " << fileName <<"\n");

		//Read second argument (file type/permission)
		virtualAddr = kernel->machine->ReadRegister(5);
		int type = (int)virtualAddr;

		int result;
		result = kernel->openingFiles->Open(fileName, type);

		//Deallocate memory
		delete[] fileName;

		kernel->machine->WriteRegister(2, result);
		incrementProgramCounter();
		if (result == -1)
		{
			DEBUG(dbgFile, "Open file failed.\n");
		}
		else{
			DEBUG(dbgFile, "Open file successfully\n");
		}
		
		return;

		ASSERTNOTREACHED()
		break;
		}
	  
	  case SC_Read:
	  {
		DEBUG(dbgFile, "Reading file with ID:" << (int)kernel->machine->ReadRegister(6) << "; Size: "
		<< (int)kernel->machine->ReadRegister(5) <<"\n");
		
		
		int bufferVirtualAddr = kernel->machine->ReadRegister(4);
		int bufferSize = (int)kernel->machine->ReadRegister(5);
		if (bufferSize < 0)
		{
			kernel->machine->WriteRegister(2, -1);
			incrementProgramCounter();
			return;
		}
		ASSERT(bufferSize >= 0);
		int fileId = (int)kernel->machine->ReadRegister(6);

		char *fileBuffer = new char[bufferSize+1];
		
		int result = kernel->openingFiles->Read(fileBuffer, bufferSize, fileId);
		DEBUG(dbgFile, "Return value " << result << "to return value register\n");
		kernel->machine->WriteRegister(2, result);
		DEBUG(dbgFile, "Move buffer: \n" << fileBuffer << "\n....to userspace.\n");
		SystemToUser(bufferVirtualAddr, bufferSize, fileBuffer);

		delete[] fileBuffer;
		DEBUG(dbgFile, "Delete kernel space buffer.\n");
		DEBUG(dbgFile, "Moving program counter.\n");
		incrementProgramCounter();
		
		if (result == -1)
		{
			DEBUG(dbgFile, "Read file failed.\n");
		}
		else
		{
			DEBUG(dbgFile, "Read file successfully.\n");
		}
		
		return;

		ASSERTNOTREACHED()
		break;
	  }
	case SC_Write:
	  {
		DEBUG(dbgFile, "Writing file with ID:" << (int)kernel->machine->ReadRegister(6) << "; Size: "
		<< (int)kernel->machine->ReadRegister(5) <<"\n");
		
		
		int bufferVirtualAddr = kernel->machine->ReadRegister(4);

		int bufferSize = (int)kernel->machine->ReadRegister(5);
		if (bufferSize < 0)
		{
			kernel->machine->WriteRegister(2, -1);

			return;
		}
		ASSERT(bufferSize >= 0);
		int fileId = (int)kernel->machine->ReadRegister(6);

		char *fileBuffer = new char[bufferSize+1];
		fileBuffer = UserToSystem(bufferVirtualAddr, bufferSize);
		if (fileBuffer == NULL)
		{
			DEBUG(dbgFile, "Not enough memory to read buffer");
			kernel->machine->WriteRegister(2, -1);
			incrementProgramCounter();
			return;
		}
		DEBUG(dbgFile, "File name in kernel space :" << fileBuffer << "\n");
		int result = kernel->openingFiles->Write(fileBuffer, bufferSize, fileId);

		kernel->machine->WriteRegister(2, result);

		delete[] fileBuffer;
		incrementProgramCounter();
		
		if (result == -1)
		{
			DEBUG(dbgFile, "Write file failed.\n");
		}
		else
		{
			DEBUG(dbgFile, "Write file successfully.\n");
		}
		return;

		ASSERTNOTREACHED()
		break;
	  }
	  case SC_Close:
	  {
		int id = (int)kernel->machine->ReadRegister(4);
		
		DEBUG(dbgFile, "Closing file with id:" << id << "\n");
		

		int result;
		result = kernel->openingFiles->Close(id);

		kernel->machine->WriteRegister(2, result);
		incrementProgramCounter();
		DEBUG(dbgFile, "Close file successfully\n");
		return;

		ASSERTNOTREACHED()
		break;

	  }

	  case SC_TCPSocketInit:
	  {
		
		DEBUG(dbgSys, "Init TCP socket...\n");
		int result;
		result = kernel->openingFiles->SocketTCP();
		if (result == -1)
		{
			DEBUG(dbgSys, "Init TCP socket failed.\n");
		}
		else
		{
			DEBUG(dbgSys, "Init TCP socket succesfully at " << result << ".\n");
		}
		kernel->machine->WriteRegister(2, result);
		incrementProgramCounter();
		
		return;

		ASSERTNOTREACHED()
		break;
	  }

	  case SC_SocketConnect:
	  {
		DEBUG(dbgSys, "Connect to socket with virtual ID:" << (int)kernel->machine->ReadRegister(4) << "; IP: "
		<< (int)kernel->machine->ReadRegister(5) 
		<< ", Port: " << (int)kernel->machine->ReadRegister(6) << "\n");
		
		int virtualId = (int)kernel->machine->ReadRegister(4);
		int ipVirtualAddr = kernel->machine->ReadRegister(5);

		int port = (int)kernel->machine->ReadRegister(6);
		if (port < 0)
		{
			DEBUG(dbgSys, "Invalid port.\n");
			kernel->machine->WriteRegister(2, -1);

			return;
		}
		ASSERT(port >= 0);
		int fileId = (int)kernel->machine->ReadRegister(6);

		char *ipBuffer = new char[32+1];
		ipBuffer = UserToSystem(ipVirtualAddr, 32);
		if (ipBuffer == NULL)
		{
			DEBUG(dbgFile, "Not enough memory to read buffer");
			kernel->machine->WriteRegister(2, -1);
			incrementProgramCounter();
			return;
		}
		DEBUG(dbgFile, "IP in kernel space :" << ipBuffer << "\n");
		int result = kernel->openingFiles->Connect(virtualId, ipBuffer, port);
		if (result == -1)
		{
			DEBUG(dbgFile, "Connect to socket failed\n");
		}
		else
		{
			DEBUG(dbgFile, "Connect to socket successfully\n");
		}
		kernel->machine->WriteRegister(2, result);

		delete[] ipBuffer;
		incrementProgramCounter();
		return;

		ASSERTNOTREACHED()
		break;
	  }

	  case SC_SocketSend:
	  {
		DEBUG(dbgSys, "Send to socket with virtual ID:" << (int)kernel->machine->ReadRegister(4) << "\n");
		int packetVirtualAddr = kernel->machine->ReadRegister(5);
		int packetLength = (int)kernel->machine->ReadRegister(6);
		char *packetBuffer = new char[packetLength + 1];
		packetBuffer = UserToSystem(packetVirtualAddr, packetLength);

		int virtualId = (int)kernel->machine->ReadRegister(4);
		DEBUG(dbgSys, "Buffer at kernel space: " << packetBuffer << "\nLength: " << packetLength << "\n");

		if (packetLength < 0)
		{
			DEBUG(dbgSys, "Invalid packet length.\n");
			kernel->machine->WriteRegister(2, -1);

			return;
		}
		ASSERT(packetLength >= 0);
		int fileId = (int)kernel->machine->ReadRegister(6);

		
		if (packetBuffer == NULL)
		{
			DEBUG(dbgFile, "Not enough memory to read buffer");
			kernel->machine->WriteRegister(2, -1);
			incrementProgramCounter();
			return;
		}
		DEBUG(dbgFile, "IP in kernel space :" << packetBuffer << "\n");
		int result = kernel->openingFiles->Send(virtualId, packetBuffer, packetLength);
		if (result == -1)
		{
			DEBUG(dbgFile, "Send to socket failed\n");
		}
		else
		{
			DEBUG(dbgFile, "send to socket successfully\n");
		}
		kernel->machine->WriteRegister(2, result);

		delete[] packetBuffer;
		incrementProgramCounter();
		return;

		ASSERTNOTREACHED()
		break;
	  }

	  case SC_SocketReceive:
	  {
		DEBUG(dbgSys, "Receive from socket with virtual ID:" << (int)kernel->machine->ReadRegister(4) << "\n");
		int packetVirtualAddr = kernel->machine->ReadRegister(5);
		int packetLength = (int)kernel->machine->ReadRegister(6);
		char *packetBuffer = new char[packetLength + 1];

		int virtualId = (int)kernel->machine->ReadRegister(4);
		DEBUG(dbgSys, "Receive buffer address: " << packetVirtualAddr << "\nLength: " << packetLength << "\n");

		if (packetLength < 0)
		{
			DEBUG(dbgSys, "Invalid packet length.\n");
			kernel->machine->WriteRegister(2, -1);

			return;
		}
		ASSERT(packetLength >= 0);
		int fileId = (int)kernel->machine->ReadRegister(6);

		
		if (packetBuffer == NULL)
		{
			DEBUG(dbgFile, "Not enough memory to read buffer");
			kernel->machine->WriteRegister(2, -1);
			incrementProgramCounter();
			return;
		}
		int result = kernel->openingFiles->Receive(virtualId, packetBuffer, packetLength);
		if (result == -1)
		{
			DEBUG(dbgFile, "Receive socket failed\n");
		}
		else
		{
			DEBUG(dbgFile, "send to socket successfully\n");
		}
		SystemToUser(packetVirtualAddr, result, packetBuffer);
		kernel->machine->WriteRegister(2, result);

		delete[] packetBuffer;
		incrementProgramCounter();
		return;

		ASSERTNOTREACHED()
		break;
	  }

	  case SC_Seek:
	  {
		DEBUG(dbgFile, "Move to file cursor of file with ID:" << (int)kernel->machine->ReadRegister(5) << "; Position: "
		<< (int)kernel->machine->ReadRegister(4) <<"\n");
		
		
		int position = (int)kernel->machine->ReadRegister(4);
		if (position < 0 && position != -1)
		{
			kernel->machine->WriteRegister(2, -1);
			incrementProgramCounter();
			return;
		}
		ASSERT(position >= -1);
		int fileId = (int)kernel->machine->ReadRegister(5);
		
		int result = kernel->openingFiles->Seek(position, fileId);

		kernel->machine->WriteRegister(2, result);
		incrementProgramCounter();
		return;

		ASSERTNOTREACHED()
		break;
	  }

	case SC_Remove:
	  {
	
		int virtualAddr = kernel->machine->ReadRegister(4);
		char *fileName = UserToSystem(virtualAddr, FileNameMaxLen+1);
		DEBUG(dbgFile, "Removing files:" << fileName << "\n");

		if (fileName == NULL)
		{
			DEBUG(dbgFile, "Not enough memory on system to create file.\n");
			kernel->machine->WriteRegister(2, -1);
			incrementProgramCounter();
			return;
		}
		DEBUG(dbgFile, "Finished reading file name: " << fileName <<"\n");
		int result;
		result = kernel->openingFiles->Remove(fileName);
		delete[] fileName;

		kernel->machine->WriteRegister(2, result);
		incrementProgramCounter();
		DEBUG(dbgFile, "Remove file successfully: \n");
		return;

		ASSERTNOTREACHED()
		break;
	  }

      default:
	  {
		cerr << "Unexpected system call " << type << "\n";
		break;
	  }
      }
      break;
    default:
      cerr << "Unexpected user mode exception" << (int)which << "\n";
      break;
	  
    }
    ASSERTNOTREACHED();
}
