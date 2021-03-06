// msgHook.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

#include "mhook-lib/mhook.h"
#include <stdio.h>
//////////////////////////////////////////////////////////////////////////
#include "ShareMem/ShareMemry.h"
#include <map>

using std::map;
using COMMUSE::ShareMemry;

#define		MEM_SHARE_FILE_NAME		"test_rule_pid_collect_map"
#define		MAX_SHARE_MEM_SIZE	10000

//////////////////////////////////////////////////////////////////////////
ShareMemry gShareMem;
map<DWORD,ShareMemNode> mapShareInfo;

//////////////////////////////////////////////////////////////////////////

void WriteLog(const char* pFormat, ...);
BOOL StopHook ();
void LoadMapData();

#pragma data_seg("MYMSGPICKHOOKDATA")//Shared data among all instances.
HHOOK	g_hHook = NULL;
#pragma data_seg()
//////////////////////////////////////////////////////////////////////////
HINSTANCE g_hInstance = NULL;
// 空的钩子函数 
LRESULT WINAPI MyHook(int nCode,WPARAM wParam,LPARAM lParam) 
{ 
	return(CallNextHookEx(g_hHook,nCode,wParam,lParam)); 
} 
//--------------------------------------------------------------------------- 
// 输出,安装空的钩子函数 
extern"C"__declspec(dllexport) BOOL GetVer() 
{ 
	//	g_main_pid  = GetCurrentProcessId();
	if( NULL == g_hInstance)
	{
		return FALSE;
	}
	g_hHook = SetWindowsHookEx(WH_CBT, MyHook, g_hInstance, 0);
	if( NULL == g_hHook)
	{
		DWORD dwError = ::GetLastError();
		WriteLog("SetWindowsHookEx failed[err=%u]", dwError);
		return FALSE;
	}
	return TRUE; 
} 
//--------------------------------------------------------------------------- 
// 输出,Uninstall钩子函数 
extern"C"__declspec(dllexport) BOOL SetVer() 
{   
	StopHook();
	UnhookWindowsHookEx(g_hHook);
	return TRUE; 
} 

//////////////////////////////////////////////////////////////////////////

#define			MEM_SHARE_FILE_NAME		"test_rule_pid_collect_map"

//////////////////////////////////////////////////////////////////////////
#ifdef _NEED_THREAD_MSG
typedef   enum   _THREADINFOCLASS   {  
        ThreadBasicInformation,  
        ThreadTimes,  
        ThreadPriority,  
        ThreadBasePriority,  
        ThreadAffinityMask,  
        ThreadImpersonationToken,  
        ThreadDescriptorTableEntry,  
        ThreadEnableAlignmentFaultFixup,  
        ThreadEventPair_Reusable,  
        ThreadQuerySetWin32StartAddress,  
        ThreadZeroTlsCell,  
        ThreadPerformanceCount,  
        ThreadAmILastThread,  
        ThreadIdealProcessor,  
        ThreadPriorityBoost,  
        ThreadSetTlsArrayAddress,  
        ThreadIsIoPending,  
        ThreadHideFromDebugger,  
        ThreadBreakOnTermination,  
        MaxThreadInfoClass  
}   THREADINFOCLASS;  


  typedef struct _CLIENT_ID {  
    HANDLE   UniqueProcess;  
    HANDLE   UniqueThread;  
} CLIENT_ID; 
typedef   CLIENT_ID   *PCLIENT_ID;  

typedef   struct   _THREAD_BASIC_INFORMATION   {   //   Information   Class   0  
    LONG        ExitStatus;  
    PVOID       TebBaseAddress;  
    CLIENT_ID   ClientId;  
    LONG        AffinityMask;  
    LONG        Priority;  
    LONG        BasePriority;  
}   THREAD_BASIC_INFORMATION,   *PTHREAD_BASIC_INFORMATION;  
 typedef LONG (__stdcall * PFN_ZwQueryInformationThread)(  
                                                       IN   HANDLE   ThreadHandle,  
                                                       IN   THREADINFOCLASS   ThreadInformationClass,  
                                                       OUT   PVOID   ThreadInformation,  
                                                       IN   ULONG   ThreadInformationLength,  
                                                       OUT   PULONG   ReturnLength
                                                       );
//////////////////////////////////////////////////////////////////////////
PFN_ZwQueryInformationThread pfn_ZwQueryInformationThread = NULL; 
#endif


typedef	LRESULT (WINAPI *NTDEVICESENDMESSAGE)(
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam);

typedef	LRESULT (WINAPI *NTDEVICEPOSTMESSAGE)(
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam);

typedef	BOOL (WINAPI *NTDEVICEPOSTTHREADMESSAGE)( IN DWORD idThread, IN UINT Msg, IN WPARAM wParam, IN LPARAM lParam);

NTDEVICESENDMESSAGE Real_NtSendMessageA = NULL;
NTDEVICESENDMESSAGE Real_NtSendMessageW = NULL;

NTDEVICEPOSTMESSAGE Real_NtPostMessageA = NULL;
NTDEVICEPOSTMESSAGE Real_NtPostMessageW = NULL;

#ifdef _NEED_THREAD_MSG
NTDEVICEPOSTTHREADMESSAGE Real_NtPostThreadMessageA = NULL;
NTDEVICEPOSTTHREADMESSAGE Real_NtPostThreadMessageW = NULL;
#endif

bool g_exit = false;

//返回成功，表求允许发送消息
//返回失败，不允许发送消息
bool JudgeRule(DWORD dwPid)
{
	DWORD dwPidSelf = ::GetCurrentProcessId();

	if (mapShareInfo.find(dwPidSelf)==mapShareInfo.end())
	{
		LoadMapData();
	}

	if (mapShareInfo.find(dwPid)==mapShareInfo.end())
	{
		LoadMapData();
	}

	//////////////////////////////////////////////////////////////////////////
	auto itParent = mapShareInfo.find(dwPidSelf);
	if( itParent == mapShareInfo.end() ) return true;

	auto itChild = mapShareInfo.find(dwPid);
	if( itChild == mapShareInfo.end() ) return true;

	if(itParent->second.bTrust) return true;
	if(itChild->second.bTrust == false) return true;
	//////////////////////////////////////////////////////////////////////////
	return false;
}

bool SelfTrust()
{
	DWORD dwPidSelf = ::GetCurrentProcessId();

	if (mapShareInfo.find(dwPidSelf)==mapShareInfo.end())
	{
		Sleep(100);
		LoadMapData();
	}

	if (mapShareInfo.find(dwPidSelf)==mapShareInfo.end())
	{
		Sleep(100);
		LoadMapData();
	}

	//////////////////////////////////////////////////////////////////////////
	auto it = mapShareInfo.find(dwPidSelf);
	if( it == mapShareInfo.end() ) return false;

	//////////////////////////////////////////////////////////////////////////
	return it->second.bTrust;
}


bool gCanSendMsg(HWND hWnd)
{
	if( NULL == hWnd)
	{
		return true;
	}
	DWORD dwPid = 0;
	GetWindowThreadProcessId(hWnd, &dwPid);
	DWORD dwMy = ::GetCurrentProcessId();
	if(0 == dwPid)
	{
		return true;
	}
	if(dwMy == dwPid)
	{
		return true;
	}
	return JudgeRule(dwPid);
}

#ifdef _NEED_THREAD_MSG
bool gCanSendThreadMsg(DWORD dwThreadId)
{
	HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, dwThreadId);
	if(INVALID_HANDLE_VALUE == hThread )
	{
		return true;
	}
	if( NULL == hThread)
	{
		return true;
	}
	if( NULL == pfn_ZwQueryInformationThread)
	{
		return true;
	}
	THREAD_BASIC_INFORMATION tbi;
	LONG status = 0;
	ULONG uRet = 0;
	status = pfn_ZwQueryInformationThread(hThread, ThreadBasicInformation, &tbi, sizeof(tbi), &uRet);
	CloseHandle(hThread);
	if( 0 != status)
	{
		return true;
	}

	DWORD dwPid = (DWORD)(tbi.ClientId.UniqueProcess);
	DWORD dwMy = ::GetCurrentProcessId();
	WriteLog("gCanSendThreadMsg enter[%u-me=%u].", dwPid, dwMy);
	if(0 == dwPid)
	{
		return true;
	}
	if(dwMy == dwPid)
	{
		return true;
	}
	return JudgeRule(dwPid);
}
#endif


LRESULT WINAPI Hook_SendMessageA(
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
#ifdef _TIP_LOG
	WriteLog("Hook_SendMessageA enter.");
#endif
	bool bRet = gCanSendMsg(hWnd);
	if(!bRet)
	{
		return 0;
	}
	return Real_NtSendMessageA(hWnd, Msg, wParam, lParam);
}


LRESULT WINAPI Hook_SendMessageW(
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
#ifdef _TIP_LOG
	WriteLog("Hook_SendMessageW enter.");
#endif
	bool bRet = gCanSendMsg(hWnd);
	if(!bRet)
	{
		return 0;
	}
	return Real_NtSendMessageW(hWnd, Msg, wParam, lParam);
}



LRESULT WINAPI Hook_PostMessageA(
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
#ifdef _TIP_LOG
	WriteLog("Hook_PostMessageA enter.");
#endif
	bool bRet = gCanSendMsg(hWnd);
	if(!bRet)
	{
		return 0;
	}
	return Real_NtPostMessageA(hWnd, Msg, wParam, lParam);
}


LRESULT WINAPI Hook_PostMessageW(
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
#ifdef _TIP_LOG
	WriteLog("Hook_PostMessageW enter.");
#endif
	bool bRet = gCanSendMsg(hWnd);
	if(!bRet)
	{
		return 0;
	}
	return Real_NtPostMessageW(hWnd, Msg, wParam, lParam);
}

#ifdef _NEED_THREAD_MSG
BOOL WINAPI Hook_PostThreadMessageA(
    IN DWORD idThread,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
#ifdef _TIP_LOG
	WriteLog("Hook_PostThreadMessageA enter.");
#endif
	bool bRet = gCanSendThreadMsg(idThread);
	if(!bRet)
	{
		return 0;
	}
	return Real_NtPostThreadMessageA(idThread, Msg, wParam, lParam);
}

BOOL WINAPI Hook_PostThreadMessageW(
    IN DWORD idThread,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
#ifdef _TIP_LOG
	WriteLog("Hook_PostThreadMessageW enter.");
#endif
	bool bRet = gCanSendThreadMsg(idThread);
	if(!bRet)
	{
		return 0;
	}
	return Real_NtPostThreadMessageW(idThread, Msg, wParam, lParam);
}
#endif
//////////////////////////////////////////////////////////////////////////
BOOL	StopHook()
{
	if (Real_NtSendMessageA)
	{
		Mhook_Unhook((PVOID*)Real_NtSendMessageA);
		Real_NtSendMessageA = NULL;
	}
	if (Real_NtSendMessageW)
	{
		Mhook_Unhook((PVOID*)Real_NtSendMessageW);
		Real_NtSendMessageW = NULL;
	}

	if (Real_NtPostMessageA)
	{
		Mhook_Unhook((PVOID*)Real_NtPostMessageA);
		Real_NtPostMessageA = NULL;
	}
	if (Real_NtPostMessageW)
	{
		Mhook_Unhook((PVOID*)Real_NtPostMessageW);
		Real_NtPostMessageW = NULL;
	}
#ifdef _NEED_THREAD_MSG
	if (Real_NtPostThreadMessageA)
	{
		Mhook_Unhook((PVOID*)Real_NtPostThreadMessageA);
		Real_NtPostThreadMessageA = NULL;
	}
	if (Real_NtPostThreadMessageW)
	{
		Mhook_Unhook((PVOID*)Real_NtPostThreadMessageW);
		Real_NtPostThreadMessageW = NULL;
	}
#endif
	return FALSE;
}


//提升权限
bool SetPrivilege()
{
	///提升权限
	TOKEN_PRIVILEGES tkp;
	HANDLE hToken;
	if (!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
	{
		//	MessageBox("OpenProcessToken failed!");
		DWORD err = GetLastError();
		/*CDebugLog log;
		log.SaveLog(1, NULL, "OpenProcessToken err = %d", err); */
		return false;
	}
	if(! LookupPrivilegeValue(NULL, SE_DEBUG_NAME,&tkp.Privileges[0].Luid))//获得本地机唯一的标识
	{ 
		//	AfxMessageBox("从本地计算机提取权限失败！");
		DWORD err = GetLastError();
		/*CDebugLog log;
		log.SaveLog(1, NULL, "LookupPrivilegeValue err = %d", err); */
		CloseHandle(hToken);
		return false;
	}
	tkp.PrivilegeCount = 1;  
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
	AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp),(PTOKEN_PRIVILEGES) NULL, 0); //调整获得的权限
	if (GetLastError() != ERROR_SUCCESS) 
	{
		DWORD err = GetLastError();
		/*CDebugLog log;
		log.SaveLog(1, NULL, "AdjustTokenPrivileges err = %d", err);*/ 
		CloseHandle(hToken);
		//	MessageBox("AdjustTokenPrivileges enable failed!");
		return false;
	}

	CloseHandle(hToken);

	return true;
}


DWORD __stdcall StartAddress(LPVOID lParam) 
{
	WriteLog("StartAddress enter.");

	if( SelfTrust() )
	{
		WriteLog("self is trusted.");
		return 0;
	}

	
    HMODULE hModule = LoadLibrary("user32.dll");
    if (hModule == NULL)
    {
		WriteLog("load user32.dll failed.");
        return 0;
    } 
#ifdef _NEED_THREAD_MSG
	HMODULE hNtdllModule = GetModuleHandle("ntdll");
	if( NULL != hNtdllModule)
	{
		pfn_ZwQueryInformationThread = (PFN_ZwQueryInformationThread)
        GetProcAddress(hNtdllModule, "ZwQueryInformationThread"); 
	}
	if( NULL != pfn_ZwQueryInformationThread)
	{
		WriteLog("load ntdll.dll pfn_ZwQueryInformationThread success.");
	}
#endif
	SetPrivilege();
	Real_NtSendMessageA = (NTDEVICESENDMESSAGE)GetProcAddress(hModule,"SendMessageA");

    if (Real_NtSendMessageA != NULL)
    { 

        if( ! Mhook_SetHook((PVOID*)&Real_NtSendMessageA, Hook_SendMessageA))
        { 
            Real_NtSendMessageA = NULL;
        } 
    }
	Real_NtSendMessageW = (NTDEVICESENDMESSAGE)GetProcAddress(hModule,"SendMessageW");

    if (Real_NtSendMessageW != NULL)
    { 

        if( ! Mhook_SetHook((PVOID*)&Real_NtSendMessageW, Hook_SendMessageW))
        { 
            Real_NtSendMessageW = NULL;
        } 
    }
	//////////////////////////////////////////////////////////////////////////
	Real_NtPostMessageA = (NTDEVICEPOSTMESSAGE)GetProcAddress(hModule,"PostMessageA");

    if (Real_NtPostMessageA != NULL)
    { 

        if( ! Mhook_SetHook((PVOID*)&Real_NtPostMessageA, Hook_PostMessageA))
        { 
            Real_NtPostMessageA = NULL;
        } 
    }
	Real_NtPostMessageW = (NTDEVICEPOSTMESSAGE)GetProcAddress(hModule,"PostMessageW");

    if (Real_NtPostMessageW != NULL)
    { 

        if( ! Mhook_SetHook((PVOID*)&Real_NtPostMessageW, Hook_PostMessageW))
        { 
            Real_NtPostMessageW = NULL;
        } 
    }
#ifdef _NEED_THREAD_MSG
	Real_NtPostThreadMessageA = (NTDEVICEPOSTTHREADMESSAGE)GetProcAddress(hModule,"PostThreadMessageA");

    if (Real_NtPostThreadMessageA != NULL)
    { 

        if( ! Mhook_SetHook((PVOID*)&Real_NtPostThreadMessageA, Hook_PostThreadMessageA))
        { 
            Real_NtPostThreadMessageA = NULL;
        } 
    }
	Real_NtPostThreadMessageW = (NTDEVICEPOSTTHREADMESSAGE)GetProcAddress(hModule,"PostThreadMessageW");

    if (Real_NtPostThreadMessageW != NULL)
    { 

        if( ! Mhook_SetHook((PVOID*)&Real_NtPostThreadMessageW, Hook_PostThreadMessageW))
        { 
            Real_NtPostThreadMessageW = NULL;
        } 
    }
#endif
	if( NULL != Real_NtSendMessageA)
	{
		WriteLog("Real_NtSendMessageA successed.");
	}
	if( NULL != Real_NtSendMessageW)
	{
		WriteLog("Real_NtSendMessageW successed.");
	}
	if( NULL != Real_NtPostMessageA)
	{
		WriteLog("Real_NtPostMessageA successed.");
	}
	if( NULL != Real_NtPostMessageW)
	{
		WriteLog("Real_NtPostMessageW successed.");
	}
#ifdef _NEED_THREAD_MSG
	if( NULL != Real_NtPostThreadMessageA)
	{
		WriteLog("Real_NtPostThreadMessageA successed.");
	}
	if( NULL != Real_NtPostThreadMessageW)
	{
		WriteLog("Real_NtPostThreadMessageW successed.");
	}
#endif
	WriteLog("wait exit thread.");
	/*
	while(!g_exit)
	{
		Sleep(1000);
	}
	*/
	WriteLog("start exit thread.");
//	StopHook();
	FreeLibrary(hModule);
	WriteLog("exit successed.");
	return 0;
}


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		g_hInstance = (HINSTANCE)hModule;
		WriteLog("DLL_PROCESS_ATTACH enter.");
		CreateThread(NULL, 0, StartAddress, NULL, 0, NULL);
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		g_exit = true;
		WriteLog("DLL_PROCESS_DETACH enter.");
		
		Sleep(2000);
		break;
	}
    return TRUE;
}


void WriteLog(const char* pFormat, ...)
{
	if( NULL == pFormat)
	{
		return;
	}
	char cLogText[1024];
	memset(cLogText, 0, sizeof(cLogText));
	va_list vaList;
	va_start(vaList, pFormat);
	_vsnprintf(cLogText, sizeof(cLogText) - 1, pFormat,
		vaList);
	va_end(vaList);
	FILE* fp = fopen("c:\\msghook.log", "a+");
	if( NULL == fp)
	{
		return;
	}
	fwrite(cLogText, 1, strlen(cLogText), fp);
	fwrite("\r\n", 1, 2, fp);
	fclose(fp);
}



void LoadMapData()
{
	void * pBase = gShareMem.OpenShareMemry(MEM_SHARE_FILE_NAME);
	if (pBase)
	{
		BYTE cbCanRead = *((char*)pBase);
		if (cbCanRead == 0)
		{
			mapShareInfo.clear();

			ShareMemNode* pBaseNode = (ShareMemNode*)((char*)pBase+1);

			for (int i=0;i<MAX_SHARE_MEM_SIZE;i++)
			{
				if (pBaseNode->dwPid ==0 ) break;

				WriteLog("pid:%d trust:%d\r\n",pBaseNode->dwPid,pBaseNode->bTrust);
				mapShareInfo[pBaseNode->dwPid] = *pBaseNode;

				pBaseNode++;
			}
		}

		gShareMem.CloseShareMemry();
	}
}



