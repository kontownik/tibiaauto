// MemUtil.cpp: implementation of the CMemUtil class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MemUtil.h"
#include "psapi.h"
#include <stdexcept>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif // ifdef _DEBUG

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMemUtil::CMemUtil()
{
	m_prevProcessHandle = NULL;
	m_prevProcessId = -1L;
	m_prevProcessIdBase = -1L;
	m_prevBaseAddr = NULL;
	m_globalProcessId = -1L;
	m_globalBaseAddr = NULL;
	LARGE_INTEGER perfFreq;
	QueryPerformanceFrequency(&perfFreq);
	m_cacheTimeoutTicks = (perfFreq.QuadPart * MEMORY_CACHE_VALID_TIME) / 1000;
}

BOOL CMemUtil::AdjustPrivileges()
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tp;
	TOKEN_PRIVILEGES oldtp;
	DWORD dwSize = sizeof(TOKEN_PRIVILEGES);
	LUID luid;


	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		AfxMessageBox("ERROR: Unable to open process token");
		PostQuitMessage(0);
		return 0;
	}

	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
	{
		AfxMessageBox("ERROR: Unable to lookup debug privilege");
		CloseHandle(hToken);
		PostQuitMessage(0);
		return 0;
	}

	ZeroMemory(&tp, sizeof(tp));
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	/* Adjust Token Privileges */
	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &oldtp, &dwSize))
	{
		AfxMessageBox("ERROR: Unable to adjust token privileges");
		CloseHandle(hToken);
		PostQuitMessage(0);
		return 0;
	}

	CloseHandle(hToken);

	return 1;
}

HANDLE CMemUtil::gethandle(long processId)
{
	HANDLE dwHandle;

	if (m_prevProcessId != processId)
	{
		dwHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
		if (dwHandle == NULL)
		{
			m_prevProcessId = -1;
			return NULL;
		}
		m_prevProcessId = processId;
		m_prevProcessHandle = dwHandle;
	}
	else
	{
		dwHandle = m_prevProcessHandle;
	}
	return dwHandle;
}

int CMemUtil::readmemory(DWORD processId, DWORD memAddress, char* result, DWORD size, bool addBaseAddress, bool useCache /*= true*/)
{
	HANDLE dwHandle = gethandle(processId);
	if (dwHandle == NULL)
		return 1;
	void *ptr;
	if (addBaseAddress)
		ptr = (void *)(memAddress - 0x400000 + GetProcessBaseAddr(processId));
	else
		ptr = (void *)memAddress;
	if (useCache)
	{
		DWORD alignedAddr = ((DWORD)ptr) & (0xFFFFFFFF - (MEMORY_CACHE_ENTRY_SIZE - 1));
		DWORD alignOffset = ((DWORD)ptr) & (MEMORY_CACHE_ENTRY_SIZE - 1);
		DWORD bufOffset = 0;
		LARGE_INTEGER currentTimestamp;
		QueryPerformanceCounter(&currentTimestamp);
		while(bufOffset < size)
		{
			if (m_memoryCache[alignedAddr].expirationTime < currentTimestamp.QuadPart)
			{
				// Refresh the cache entry
				DWORD ret = readmemory(processId, alignedAddr, m_memoryCache[alignedAddr].value, MEMORY_CACHE_ENTRY_SIZE, 0, false);
				if (ret != 0)
					return ret;
				m_memoryCache[alignedAddr].expirationTime = currentTimestamp.QuadPart + m_cacheTimeoutTicks;
			}
			DWORD entryReadSize = min(size - bufOffset, MEMORY_CACHE_ENTRY_SIZE - alignOffset);
			// Load the entry data
			memcpy(result + bufOffset, (m_memoryCache[alignedAddr].value + alignOffset), entryReadSize);
			// After the first load, there's no longer an align
			alignOffset = 0;
			alignedAddr += MEMORY_CACHE_ENTRY_SIZE;
			bufOffset += entryReadSize;
		}
		return 0;
	}
	if (ReadProcessMemory(dwHandle, ptr, result, size, NULL))
	{
		return 0;
	}
	else
	{
		if (::GetLastError() == ERROR_INVALID_HANDLE)
		{
			//FILE *f=fopen("C:/out.txt","a+");
			//fprintf(f,"time %lld old %d,",time(NULL),dwHandle);
			dwHandle = NULL;
			for (int iter = 1000; iter > 0; iter--)
			{
				dwHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_prevProcessId);
				if (dwHandle)
				{
					char buf[111];
					sprintf(buf, "iter %d", iter);
					if (iter != 1000)
						MessageBox(NULL, buf, "", 0);
					break;
				}
				Sleep(10);
			}

			//fprintf(f,"new %d\n",dwHandle);
			//fclose(f);
			m_prevProcessHandle = dwHandle;
			if (ReadProcessMemory(dwHandle, ptr, result, size, NULL))
				return 0;
		}
		if (::GetLastError() == ERROR_NOACCESS)
		{
			CloseHandle(dwHandle);
			dwHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, m_prevProcessId);
			m_prevProcessHandle = dwHandle;
			if (ReadProcessMemory(dwHandle, ptr, result, size, NULL))
				return 0;
		}
		if (::GetLastError() == ERROR_PARTIAL_COPY)
		{
			//Possibly Tibia has been killed
			//Test valid address; taCMemUtil::GetMemIntValue(-1)
			void *ptrTest = (void*)(0x0410000 + GetProcessBaseAddr(processId));
			int resultTest;
			if (!ReadProcessMemory(dwHandle, ptrTest, &resultTest, 4, NULL))
			{
				//try getting new handle
				dwHandle = NULL;
				for (int iter = 1000; iter > 0; iter--)
				{
					dwHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_prevProcessId);
					if (dwHandle)
					{
						char buf[111];
						sprintf(buf, "iter %d", iter);
						if (iter != 1000)
							MessageBox(NULL, buf, "", 0);
						break;
					}
					Sleep(10);
				}
				m_prevProcessHandle = dwHandle;
				if (ReadProcessMemory(dwHandle, ptr, result, size, NULL))
				{
					return 0;
				}
				else
				{
					DWORD terminatedStatus = 9999;
					if (GetExitCodeProcess(dwHandle, &terminatedStatus))
						if (terminatedStatus != STILL_ACTIVE)  //If Tibia is no longer active then close TA
							PostQuitMessage(0);
				}
			}
			else
			{
				//Tibia is still running but a bad memory address was given
				return 1;
			}


			//fprintf(f,"new %d\n",dwHandle);
			//fclose(f);
		}
		DWORD err = ::GetLastError();
		CloseHandle(dwHandle);
		m_prevProcessId = -1;
		PostQuitMessage(0);
		return err;
	}
}

int CMemUtil::writememory(DWORD processId, DWORD memAddress, int* value, DWORD size, bool addBaseAddress, bool useCache /*= true*/)
{
	HANDLE dwHandle = gethandle(processId);
	if (dwHandle == NULL)
		return 1;

	void *ptr;
	if (addBaseAddress)
		ptr = (void *)(memAddress - 0x400000 + GetProcessBaseAddr(processId));
	else
		ptr = (void *)memAddress;
	DWORD bytesWritten;
	if (useCache)
	{
		// Invalidate related cache entries
		DWORD alignedAddr = ((DWORD)ptr) & (0xFFFFFFFF - (MEMORY_CACHE_ENTRY_SIZE - 1));
		DWORD alignOffset = ((DWORD)ptr) & (MEMORY_CACHE_ENTRY_SIZE - 1);
		DWORD endAddr = ((DWORD)ptr) + size;
		while (alignedAddr < endAddr)
		{
			m_memoryCache[alignedAddr].expirationTime = 0;
			alignedAddr += MEMORY_CACHE_ENTRY_SIZE;
		}
	}
	if (WriteProcessMemory(dwHandle, ptr, value, size, &bytesWritten))
	{
		return 0;
	}
	else
	{
		if (::GetLastError() == ERROR_INVALID_HANDLE)
		{
			//FILE *f=fopen("C:/out.txt","a+");
			//fprintf(f,"time %lld old %d,",time(NULL),dwHandle);
			dwHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_prevProcessId);
			//fprintf(f,"new %d\n",dwHandle);
			//fclose(f);
			m_prevProcessHandle = dwHandle;
			if (WriteProcessMemory(dwHandle, ptr, value, size, NULL))
				return 0;
		}
		if (::GetLastError() == ERROR_NOACCESS)
		{
			dwHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, m_prevProcessId);
			m_prevProcessHandle = dwHandle;
			if (WriteProcessMemory(dwHandle, ptr, value, size, NULL))
				return 0;
		}
		DWORD err = ::GetLastError();
		CloseHandle(dwHandle);
		m_prevProcessId = -1;
		return err;
	}
}

int CMemUtil::GetProcessBaseAddr(int processId)
{
	HANDLE dwHandle = gethandle(processId);
	if (processId == m_prevProcessIdBase && m_prevProcessIdBase != -1 && m_prevBaseAddr != NULL)
	{
		return m_prevBaseAddr;
	}
	else
	{
		m_prevProcessIdBase = -1;
		m_prevBaseAddr = NULL;
	}

	int ret = 0;
	int isNotFromNormalScan = 0;
	if (dwHandle)
	{
		unsigned long moduleCount = 0;
		EnumProcessModules(dwHandle, NULL, 0, &moduleCount);
		moduleCount = moduleCount / sizeof(HMODULE);

		HMODULE *modules = (HMODULE*)calloc(moduleCount, sizeof(HMODULE));
		char moduleName[64];
		EnumProcessModules(dwHandle, modules, moduleCount * sizeof(HMODULE), &moduleCount);
		for (unsigned long i = 0; i < moduleCount; i++)
		{
			GetModuleBaseName(dwHandle, modules[i], moduleName, sizeof(moduleName));
			if (_strcmpi(moduleName, "Tibia.exe") == 0)
			{
				MODULEINFO moduleInfo;
				GetModuleInformation(dwHandle, modules[i], &moduleInfo, sizeof(moduleInfo));
				//isNotFromNormalScan=0; // commented to see if Tibia.exe in sometimes not first
				ret = (int)moduleInfo.lpBaseOfDll;
				break;
			}
			if (i == 0) // catches first module in case Tibia.exe does not exist
			{
				MODULEINFO moduleInfo;
				GetModuleInformation(dwHandle, modules[i], &moduleInfo, sizeof(moduleInfo));
				isNotFromNormalScan = 1;
				ret = (int)moduleInfo.lpBaseOfDll;
			}
		}
		free(modules);
		modules = NULL;
	}
	if (isNotFromNormalScan)
		AfxMessageBox("While finding base address, main module was no first or was not named \"Tibia.exe\".");
	if (ret)
	{
		m_prevBaseAddr = ret;
		m_prevProcessIdBase = processId;
	}
	return ret;
}

int CMemUtil::GetMemIntValue(long processId, DWORD memAddress, long int *value, bool addBaseAddress, bool useCache /*= true*/)
{
	return readmemory(processId, memAddress, (char*)value, sizeof(long int), addBaseAddress, useCache);
}

int CMemUtil::GetMemRange(long processId, DWORD memAddressStart, DWORD memAddressEnd, char *result, bool addBaseAddress, bool useCache /*= true*/)
{
	return readmemory(processId, memAddressStart, result, memAddressEnd - memAddressStart, addBaseAddress, useCache);
}

void CMemUtil::GetMemRange(DWORD memAddressStart, DWORD memAddressEnd, char *ret, bool addBaseAddress /*= true*/, bool useCache /*= true*/)
{
	GetMemRange(m_globalProcessId, memAddressStart, memAddressEnd, ret, addBaseAddress, useCache);
};

long int CMemUtil::GetMemIntValue(DWORD memAddress, bool addBaseAddress /*= true*/, bool useCache /*= true*/)
{
	long int value;
	int ret = CMemUtil::GetMemIntValue(m_globalProcessId, memAddress, &value, addBaseAddress, useCache);
	if (ret != 0)
	{
		char buf[128];
		sprintf(buf, "ERROR: read memory failed; error=%d", ret);
		throw std::runtime_error("Error reading Tibia memory.");//PostQuitMessage(0);
		return 0;
	}
	return value;
};

int CMemUtil::SetMemIntValue(DWORD memAddress, long int value, bool addBaseAddress /*= true*/, bool useCache /*= true*/)
{
	return SetMemIntValue(m_globalProcessId, memAddress, value, addBaseAddress, useCache);
}

int CMemUtil::SetMemIntValue(long processId, DWORD memAddress, long int value, bool addBaseAddress, bool useCache /*= true*/)
{
	return writememory(processId, memAddress, (int*)&value, sizeof(long int), addBaseAddress, useCache);
}

int CMemUtil::SetMemByteValue(long processId, DWORD memAddress, char value, bool addBaseAddress, bool useCache /*= true*/)
{
	return writememory(processId, memAddress, (int*)&value, sizeof(char), addBaseAddress, useCache);
}

int CMemUtil::SetMemRange(DWORD memAddressStart, DWORD memAddressEnd, char *data, bool addBaseAddress /*= true*/, bool useCache /*= true*/)
{
	return SetMemRange(m_globalProcessId, memAddressStart, memAddressEnd, data, addBaseAddress, useCache);
}

int CMemUtil::SetMemRange(int processId, DWORD memAddressStart, DWORD memAddressEnd, char *data, bool addBaseAddress, bool useCache /*= true*/)
{
	return writememory(processId, memAddressStart, (int*)data, memAddressEnd - memAddressStart, addBaseAddress, useCache);
}
