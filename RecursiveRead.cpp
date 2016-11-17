#include "stdafx.h"
#include <windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>
#include <iostream>
#include <fstream>
using namespace std;
#pragma comment(lib, "User32.lib")

void DisplayErrorBox(LPTSTR lpszFunction);
DWORD processDirectory(TCHAR szDir[MAX_PATH]);
boolean readFile(TCHAR szFile[MAX_PATH]);
template <class T> bool startsWith(const T &s, const T &t);

int _tmain(int argc, TCHAR *argv[])
{
	TCHAR szDir[MAX_PATH];
	size_t length_of_arg;

	DWORD dwError = 0;

	// If the directory is not specified as a command-line argument,
	// print usage.

	if (argc != 2)
	{
		_tprintf(TEXT("\nUsage: %s <directory name>\n"), argv[0]);
		return (-1);
	}

	// Check that the input path plus 3 is not longer than MAX_PATH.
	// Three characters are for the "\*" plus NULL appended below.

	StringCchLength(argv[1], MAX_PATH, &length_of_arg);

	if (length_of_arg > (MAX_PATH - 3))
	{
		_tprintf(TEXT("\nDirectory path is too long.\n"));
		return (-1);
	}

	_tprintf(TEXT("\nTarget directory is %s\n\n"), argv[1]);

	// Prepare string for use with FindFile functions.  First, copy the
	// string to a buffer, then append '\*' to the directory name.

	StringCchCopy(szDir, MAX_PATH, argv[1]);

	processDirectory(szDir);
}

DWORD processDirectory(TCHAR szDir[MAX_PATH])
{
	DWORD dwError = 0;
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	LARGE_INTEGER filesize;
	TCHAR szSearchDir[MAX_PATH];
	TCHAR szFound[MAX_PATH];

	StringCchCopy(szSearchDir, MAX_PATH, szDir);
	StringCchCat(szSearchDir, MAX_PATH, TEXT("\\*"));

	// Find the first file in the directory.
	hFind = FindFirstFile(szSearchDir, &ffd);

	if (INVALID_HANDLE_VALUE == hFind)
	{
		DisplayErrorBox(TEXT("FindFirstFile"));
		return dwError;
	}

	// List all the files in the directory with some info about them.

	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			_tprintf(TEXT("  <DIR>   %s\n"), ffd.cFileName);

			if ((!_wcsicmp(ffd.cFileName, L"."))
					|| (!_wcsicmp(ffd.cFileName, L"..")))
				continue;

			StringCchCopy(szFound, MAX_PATH, szDir);
			StringCchCat(szFound, MAX_PATH, TEXT("\\"));
			StringCchCat(szFound, MAX_PATH, ffd.cFileName);
			processDirectory(szFound);
		}
		else
		{
			filesize.LowPart = ffd.nFileSizeLow;
			filesize.HighPart = ffd.nFileSizeHigh;

			std::wstring file = ffd.cFileName;
			std::wstring prefix = L"log";
			if (startsWith(file, prefix))
			{
				_tprintf(TEXT(" <FILE>   %s - Ignoring\n"), ffd.cFileName);
				continue;
			}

			_tprintf(TEXT(" <FILE>   %s - %I64u bytes\n"), ffd.cFileName, filesize.QuadPart);

			StringCchCopy(szFound, MAX_PATH, szDir);
			StringCchCat(szFound, MAX_PATH, TEXT("\\"));
			StringCchCat(szFound, MAX_PATH, ffd.cFileName);
			readFile(szFound);
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
	{
		DisplayErrorBox(TEXT("FindFirstFile"));
	}

	FindClose(hFind);
	return dwError;
}

// Yields true if the string 's' starts with the string 't'.
template <class T>
bool startsWith(const T &s, const T &t)
{
	return s.size() >= t.size() &&
		std::equal(t.begin(), t.end(), s.begin());
}

int maxReadSize = (1024 * 1024); // 1 MB

boolean readFile(TCHAR szFile[MAX_PATH]) {
	streampos size;
	char * memblock;

	ifstream file(szFile, ios::in | ios::binary);
	if (!file.is_open())
	{
		cout << "Unable to open file - " << szFile << "\n";
		DisplayErrorBox(TEXT("Unable to open file"));
		return false;
	}
	
	// get length of file:
	file.seekg(0, file.end);
	size = file.tellg();
	file.seekg(0, file.beg);

	__int64 sizeLeft = size;
	memblock = new char[maxReadSize];

	int percent = 0;
	int x = 0;

	while (sizeLeft != 0)
	{
		file.read(memblock, min(sizeLeft, maxReadSize));
		sizeLeft = size - file.tellg();
		percent = (int)(100.0f - (sizeLeft * 100.0f) / size);
		std::cout << "\r" << percent << "% completed";
		std::cout.flush();
	}
	std::cout << "\n";

	delete[] memblock;
	file.close();
	return true;
}


void DisplayErrorBox(LPTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and clean up

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40)*sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}
