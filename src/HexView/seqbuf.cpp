//
//  seqbuf.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <new>
#include "sequence.h"
#include "trace.h"


bool read_data(HANDLE hFile, seqchar *buffer, size_w offset, size_w length)
{
	LARGE_INTEGER li;
	DWORD numread;

	li.QuadPart = offset;

	SetFilePointerEx(hFile, li, 0, FILE_BEGIN);
	return ReadFile(hFile, buffer, (DWORD)length * sizeof(seqchar), &numread, 0) ? true : false;
}

bool write_data(HANDLE hFile, seqchar *buffer, size_w offset, size_w length)
{
	LARGE_INTEGER li;
	DWORD numwritten;

	li.QuadPart = offset;

	SetFilePointerEx(hFile, li, 0, FILE_BEGIN);
	return WriteFile(hFile, buffer, (DWORD)length * sizeof(seqchar), &numwritten, 0) ? true : false;
}

size_w inline calc_index_base(size_w index)
{
	if(index < MEM_BLOCK_SIZE / 2)
	{
		return 0;
	}
	else
	{
		return ((index + MEM_BLOCK_SIZE / 4) & (~(MEM_BLOCK_SIZE / 2 - 1))) - (MEM_BLOCK_SIZE / 2);
	}
}

seqchar	* sequence::getptr(span *sptr)
{
	return buffer_list[sptr->buffer]->getptr(sptr->offset, sptr->length);
}

seqchar * sequence::buffer_control::getptr(size_w off, size_w len)
{
	size_t i;

	//return viewlist[0].buffer + off;
	
	// search for a buffer that already contains the requested range of data
	for(i = 0; i < MAX_VIEWS; i++)	
	{
		buffer_view *bv = &viewlist[i];
	
		if(bv->initialized && off >= bv->offset && off+len <= bv->offset+bv->length)
		{
			return bv->buffer + (off - bv->offset);
		}
	}

	// didn't find one, so map in a new view
	if(hFile)
	{
		buffer_view *bv = &viewlist[0];

		// find one that isn't initialized yet
		for(i = 0; i < MAX_VIEWS; i++)
		{
			bv = &viewlist[i];

			if(bv->initialized == false)
			{
				bv->buffer		= new seqchar[MEM_BLOCK_SIZE];
				bv->offset		= calc_index_base(off);
				bv->length		= MEM_BLOCK_SIZE;
				bv->initialized = true;
				
				read_data(hFile, bv->buffer, bv->offset, bv->length);
				return bv->buffer + (off - bv->offset);
			}
		}

		bv->offset = calc_index_base(off);
		read_data(hFile, bv->buffer, bv->offset, bv->length);

		return bv->buffer + (off - bv->offset);
	}
	
	//return buffer + off;
	return 0;
}

//
//	Initialize with a fixed buffer
//
bool sequence::buffer_control::init(const seqchar * buf, size_w len, bool copybuf) 
{
	if(copybuf && buf == 0)
		return false;

	// make sure we don't try to alloc something too big for new[]
	if(len >= 0xffffffff)
		return false;

	length	= len;
	maxsize	= len;
	ownbuf	= copybuf;

	viewlist[0].buffer = (seqchar *)buf;
	viewlist[0].length = len;
	viewlist[0].offset = 0;
	
	try 
	{
		if(buf == false || copybuf)
		{
			if((viewlist[0].buffer = new seqchar[(size_t)len]) == 0)
				return false;
		}

		// duplicate the source buffer
		if(copybuf)
		{
			memcpy(viewlist[0].buffer, buf, (size_t)len * sizeof(seqchar));
		}
	}
	catch(std::bad_alloc & ba) 
	{
		UNREFERENCED_PARAMETER(ba);
		return false;
	}

	viewlist[0].initialized = true;
	return true;
}

//
// Initialize with a fixed (empty) buffer
//
bool sequence::buffer_control::init(size_t max)
{
	if((viewlist[0].buffer = new seqchar[max]) == 0)
		return false;

	viewlist[0].initialized = true;
	viewlist[0].offset = 0;
	viewlist[0].length = max;
	
	maxsize = max;
	length  = 0;
	ownbuf  = true;
	
	return true;
}

bool sequence::buffer_control::append(const seqchar * buf, size_t len)
{
	if(length + len < maxsize)
	{
		buffer_view *bv = &viewlist[0];
		memcpy(bv->buffer + length, buf, len * sizeof(seqchar));
		length += len;
		return true;
	}
	else
	{
		return false;
	}
}

/*

bool sequence::get_file_handle(const TCHAR * filename, bool & readonly, HANDLE & hFile, size_w & length, DWORD & dwError)
{
	DWORD  dwAccess = GENERIC_READ;

	// if the file is physically readonly, then we cannot open
	// it for writing even if we wanted to (we will get 'access denied')
	if(GetFileAttributes(filename) & FILE_ATTRIBUTE_READONLY)
		readonly = true;

	if(!readonly)
		dwAccess |= GENERIC_WRITE;

	// open the file
	hFile = CreateFile(filename, dwAccess, FILE_SHARE_READ, 
		0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if(hFile == INVALID_HANDLE_VALUE)
	{
		dwError = GetLastError();

		// try again with read-only access if the file is already open
		if(dwError == ERROR_SHARING_VIOLATION || dwError == ERROR_ACCESS_DENIED)
		{
			readonly = true;
			
			hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 
				0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		}
	}

	if(hFile == INVALID_HANDLE_VALUE)
		return false;

	DWORD sizelow, sizehigh;
	sizelow = GetFileSize(hFile, &sizehigh);
	length  = (size_w)sizelow | ((size_w)sizehigh << 32);
	
	return true;
}
*/


// initialize with a file
bool sequence::buffer_control::load(const TCHAR * filename, bool reado, bool quickload)
{
	DWORD dwAccess = GENERIC_READ;
	DWORD dwError  = S_OK;
	bool  success  = false;

	readonly = reado;

	// if the file is physically readonly, then we cannot open
	// it for writing even if we wanted to (we will get 'access denied')
	if(GetFileAttributes(filename) & FILE_ATTRIBUTE_READONLY)
		readonly = true;

	if(!readonly)
		dwAccess |= GENERIC_WRITE;

	// open the file
	hFile = CreateFile(filename, dwAccess, FILE_SHARE_READ, 
		0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if(hFile == INVALID_HANDLE_VALUE)
	{
		dwError = GetLastError();

		// try again with read-only access if the file is already open
		if(dwError == ERROR_SHARING_VIOLATION || dwError == ERROR_ACCESS_DENIED)
		{
			readonly = true;
			
			hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 
				0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		}
	}

	if(hFile == INVALID_HANDLE_VALUE)
		return false;

	DWORD sizelow, sizehigh;
	sizelow = GetFileSize(hFile, &sizehigh);
	length  = (size_w)sizelow;
#ifdef SEQUENCE64
	length |= ((size_w)sizehigh << 32);
#endif
	maxsize = length;
	ownbuf  = true;

	// only quickload if file is < 10Mb
	if(quickload == false && length < 1024*1024*10)
	{
		if(init(NULL, length, false))
		{
			if(read_data(hFile, viewlist[0].buffer, 0, length))
			{
				success = true;
			}
			else
			{
				dwError = GetLastError();
			}
		}
		else
		{
			dwError = ERROR_OUTOFMEMORY;
		}

		CloseHandle(hFile);
		hFile = 0;
	}
	else
	{
		success = true;
	}

	if(!success)
	{
		delete[] viewlist[0].buffer;
		CloseHandle(hFile);
	}

	SetLastError(dwError);
	return success;
}

HANDLE sequence::_handle()
{
	if(buffer_list.size() > 1)
	{
		for(size_t i = 0; i < buffer_list.size(); i++)
		{
			buffer_control *bc = buffer_list[i];
			HANDLE h = buffer_list[i]->hFile;
			h = h;
		}
		return buffer_list[1]->hFile;
	}

	return NULL;
}

void sequence::buffer_control::free()
{
	size_t i;

	if(hFile)
		CloseHandle(hFile);

	for(i = 0; ownbuf && i < MAX_VIEWS; i++)
	{
		if(viewlist[i].initialized)
		{
			delete[] viewlist[i].buffer;
		}
	}
}

bool allocfile(HANDLE hFile, size_w length)
{
	LARGE_INTEGER set;
	LARGE_INTEGER result;

	set.QuadPart = length;
	
	// allocate space for desired file size
	if(SetFilePointerEx(hFile, set, &result, FILE_BEGIN))
	{
		if(SetEndOfFile(hFile))
		{
			return true;
		}
	}

	return false;
}

HANDLE allocfile(const TCHAR *filename, size_w length)
{
	HANDLE hFile;

	hFile = CreateFile(filename, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0);

	if(allocfile(hFile, length))
		return hFile;

	CloseHandle(hFile);
	return 0;
}

HANDLE alloctmpfile(TCHAR *tmpfilename, int namelen, size_w length)
{
	HANDLE hFile;

	if(namelen < MAX_PATH)
		return 0;

	if(GetTempPath(namelen, tmpfilename))
	{
		if(GetTempFileName(tmpfilename, TEXT("~HX"), 0, tmpfilename))
		{
			hFile = allocfile(tmpfilename, length);

			if(hFile != INVALID_HANDLE_VALUE)
			{
				return hFile;
			}

			DeleteFile(tmpfilename);
		}
	}

	return 0;
}

HANDLE createfile(const TCHAR *filename, size_w length)
{
	TCHAR tmpfile[MAX_PATH];
	const TCHAR *newname = filename;
	HANDLE hFile;

	if(filename == 0)
	{
		GetTempPath(MAX_PATH, tmpfile);
		GetTempFileName(tmpfile, TEXT("~HX"), 0, tmpfile);

		newname = tmpfile;
	}

	hFile = CreateFile(newname, GENERIC_READ|GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0);

	if(hFile != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER set;
		LARGE_INTEGER result;

		set.QuadPart = length;
	
		// allocate space for desired file size
		if(SetFilePointerEx(hFile, set, &result, FILE_BEGIN))
		{
			if(SetEndOfFile(hFile))
			{
				return hFile;
			}
		}

		CloseHandle(hFile);
	}

	if(filename == 0)
		DeleteFile(tmpfile);

	return 0;
}