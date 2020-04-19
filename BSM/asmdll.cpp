





#include "pch.h" 
#include "asmdll.h"
#include "asm.h"
#include <stdio.h>
#include <psapi.h>
#include <windows.h>
#include <process.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <io.h>
#include "ASM.h"

static HANDLE FileMapHandle = NULL;
LPVOID FileMap = NULL;
HANDLE hProcess;

DWORD InstanceID;
ARMA_SERVER_INFO* ArmaServerInfo = NULL;
LARGE_INTEGER PCF, PCS;
char OCI0[SMALSTRINGSIZE];
char OCI1[SMALSTRINGSIZE];
char OCI2[SMALSTRINGSIZE];
char OCC0[FUNCTIONSIZE];
char OCC1[FUNCTIONSIZE];
char OCC2[FUNCTIONSIZE];


#ifdef __cplusplus
#define INITIALIZER(f) \
        static void f(void); \
        struct f##_t_ { f##_t_(void) { f(); } }; static f##_t_ f##_; \
        static void f(void)
#elif defined(_MSC_VER)
#pragma section(".CRT$XCU",read)
#define INITIALIZER2_(f,p) \
        static void f(void); \
        __declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
        __pragma(comment(linker,"/include:" p #f "_")) \
        static void f(void)
#ifdef _WIN64
#define INITIALIZER(f) INITIALIZER2_(f,"")
#else
#define INITIALIZER(f) INITIALIZER2_(f,"_")
#endif
#else
#define INITIALIZER(f) \
        static void f(void) __attribute__((constructor)); \
        static void f(void)
#endif

INITIALIZER(initialize)
{
	SECURITY_ATTRIBUTES FSA;
	SECURITY_DESCRIPTOR FSD;

	InitializeSecurityDescriptor(&FSD, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&FSD, TRUE, NULL, FALSE);

	FSA.nLength = sizeof(FSA);
	FSA.lpSecurityDescriptor = &FSD;
	FSA.bInheritHandle = TRUE;

	FileMapHandle = CreateFileMapping(INVALID_HANDLE_VALUE, &FSA, PAGE_READWRITE, 0, FILEMAPSIZE, L"Global\\ASM_MapFile"); // for DS started as service

	if (FileMapHandle == NULL) FileMapHandle = CreateFileMapping(INVALID_HANDLE_VALUE, &FSA, PAGE_READWRITE, 0, FILEMAPSIZE, L"ASM_MapFile");

	if (FileMapHandle != NULL) FileMap = MapViewOfFile(FileMapHandle, FILE_MAP_WRITE, 0, 0, FILEMAPSIZE);

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());

	QueryPerformanceFrequency(&PCF);
	QueryPerformanceCounter(&PCS);


	wchar_t OCI0in[SMALSTRINGSIZE] = L"0";
	wchar_t OCI1in[SMALSTRINGSIZE] = L"0";
	wchar_t OCI2in[SMALSTRINGSIZE] = L"0";
	wchar_t OCC0in[FUNCTIONSIZE] = L"";
	wchar_t OCC1in[FUNCTIONSIZE] = L"";
	wchar_t OCC2in[FUNCTIONSIZE] = L"";



	GetPrivateProfileString(L"ASM", L"objectcountinterval0", L"30", &OCI0in[0], SMALSTRINGSIZE - 1, L".\\asm.ini");
	wcstombs(OCI0, OCI0in, SMALSTRINGSIZE);
	GetPrivateProfileString(L"ASM", L"objectcountinterval1", L"60", &OCI1in[0], SMALSTRINGSIZE - 1, L".\\asm.ini");
	wcstombs(OCI1, OCI1in, SMALSTRINGSIZE);
	GetPrivateProfileString(L"ASM", L"objectcountinterval2", L"0", &OCI2in[0], SMALSTRINGSIZE - 1, L".\\asm.ini");
	wcstombs(OCI2, OCI2in, SMALSTRINGSIZE);
	GetPrivateProfileString(L"ASM", L"objectcountcommand0", L"count entities \"\"All\"\";", &OCC0in[0], FUNCTIONSIZE - 1, L".\\asm.ini");
	wcstombs(OCC0, OCC0in, SMALSTRINGSIZE);
	GetPrivateProfileString(L"ASM", L"objectcountcommand1", L"count vehicles;", &OCC1in[0], FUNCTIONSIZE - 1, L".\\asm.ini");
	wcstombs(OCC1, OCC1in, SMALSTRINGSIZE);
	GetPrivateProfileString(L"ASM", L"objectcountcommand2", L"count allMissionObjects \"\"All\"\";", &OCC2in[0], FUNCTIONSIZE - 1, L".\\asm.ini");
	wcstombs(OCC2, OCC2in, SMALSTRINGSIZE);
}

static void finalize()
{
	if (FileMap) {
		memset(ArmaServerInfo, 0, PAGESIZE);
		FlushViewOfFile(FileMap, FILEMAPSIZE);
	}

	CloseHandle(hProcess);

	UnmapViewOfFile(FileMap);
	CloseHandle(FileMapHandle);
}


#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN64
	extern "C" BSM_API void RVExtension(char* output, int outputSize, const char* function)
#else
	extern "C" BSM_API void RVExtension(char* output, int outputSize, const char* function)
#endif

{
	char* stopstring;

	

	switch (*function)
	{

	case '0':// FPS update 
	{
		if (FileMap) {
			unsigned FPS, FPSMIN;
			FPS = strtol(&function[2], &stopstring, 10);
			FPSMIN = strtol(&stopstring[1], &stopstring, 10);
			ArmaServerInfo->SERVER_FPS = FPS;
			ArmaServerInfo->SERVER_FPSMIN = FPSMIN;
			ArmaServerInfo->TICK_COUNT = GetTickCount();
			FlushViewOfFile(ArmaServerInfo, PAGESIZE);
		}
		return;
	}

	case '1':// CPS update 
	{
		if (FileMap) {
			LARGE_INTEGER PCE;
			double tnsec;
			unsigned conditionNo;

			QueryPerformanceCounter(&PCE);
			tnsec = (double)(PCE.QuadPart - PCS.QuadPart) / (double)PCF.QuadPart;

			conditionNo = strtol(&function[2], &stopstring, 10);
			ArmaServerInfo->FSM_CE_FREQ = static_cast<int>(floor(conditionNo * 1000 / tnsec + 0.5));

			PCS = PCE;
		}
		return;
	}

	case '2':// GEN update 
	{
		if (FileMap) {
			unsigned int players, ail, air;

			players = strtol(&function[2], &stopstring, 10);
			ail = strtol(&stopstring[1], &stopstring, 10);
			air = strtol(&stopstring[1], &stopstring, 10);
			ArmaServerInfo->PLAYER_COUNT = players;
			ArmaServerInfo->AI_LOC_COUNT = ail;
			ArmaServerInfo->AI_REM_COUNT = air;

			PROCESS_MEMORY_COUNTERS pmc;
			GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
			ArmaServerInfo->MEM = pmc.WorkingSetSize;


		}
		return;
	}

	case '3':// MISSION update 
	{
		

		if (FileMapHandle) {
			
		}
		if (FileMap) {
			strcpy_s(ArmaServerInfo->MISSION, SMALSTRINGSIZE, &function[2]);
			
		}
		return;
	}

	case '4':// OBJ_COUNT_0 update 
	{
		if (FileMap) {
			unsigned obj;
			obj = strtol(&function[2], &stopstring, 10);
			ArmaServerInfo->OBJ_COUNT_0 = obj;
		}
		return;
	}

	case '5':// OBJ_COUNT_1 update 
	{
		if (FileMap) {
			unsigned obj;
			obj = strtol(&function[2], &stopstring, 10);
			ArmaServerInfo->OBJ_COUNT_1 = obj;
		}
		return;
	}

	case '6':// OBJ_COUNT_2 update 
	{
		if (FileMap) {
			unsigned obj;
			obj = strtol(&function[2], &stopstring, 10);
			ArmaServerInfo->OBJ_COUNT_2 = obj;
		}
		return;
	}

	case '9':// Initialization
	{
		if (FileMap) {
			if (ArmaServerInfo == NULL) {
				DWORD DeadTimeout = GetTickCount() - 10000;
				for (InstanceID = 0; InstanceID < MAX_ARMA_INSTANCES; InstanceID++) {
					
					#ifdef _WIN64
					ArmaServerInfo = (ARMA_SERVER_INFO*)((uintptr_t)FileMap + (InstanceID * PAGESIZE));
					#else
					ArmaServerInfo = (ARMA_SERVER_INFO*)((DWORD)FileMap + (InstanceID * PAGESIZE));
					#endif
					
					ArmaServerInfo = (ARMA_SERVER_INFO*)((uintptr_t)FileMap + (InstanceID * PAGESIZE));
					if ((ArmaServerInfo->PID == 0) || (ArmaServerInfo->TICK_COUNT < DeadTimeout)) {
						ArmaServerInfo->TICK_COUNT = DeadTimeout + 10000;
						ArmaServerInfo->PID = GetCurrentProcessId();
						strcpy_s(ArmaServerInfo->PROFILE, SMALSTRINGSIZE, &function[2]);
						break;
					}
				}
				if (InstanceID < MAX_ARMA_INSTANCES) ArmaServerInfo->MEM = 0;
			}
			else { ArmaServerInfo->MEM = 0; }
			FlushViewOfFile(ArmaServerInfo, PAGESIZE);
		}
		sprintf_s(output, OUTPUTSIZE, "_ASM_OPT=[%s,%s,%s,\"%s\",\"%s\",\"%s\"];", OCI0, OCI1, OCI2, OCC0, OCC1, OCC2);
		return;
	}

	default:
	{
		return;
	};
	};
}

#ifdef __cplusplus
	}
#endif