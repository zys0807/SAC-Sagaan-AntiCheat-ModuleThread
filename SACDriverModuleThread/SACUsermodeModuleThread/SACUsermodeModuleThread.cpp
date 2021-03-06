// SACUsermodeModuleThread.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iterator> 
#include <Windows.h>
#include <sddl.h>
#include <accctrl.h>
#include <iomanip>
#include <stdio.h>
#include <conio.h>
#include <aclapi.h>
#include <Iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#include <stdlib.h>
#include <iostream>
#include <ole2.h>
#include <WinBase.h>
#include <olectl.h>
#include <chrono>
#include <thread>
#include <ratio>
#include <tchar.h>
#include <psapi.h>
#include <TlHelp32.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <winnt.h>
#include <Winnetwk.h>
#include <tlhelp32.h>
#include <shlwapi.h>
#include <algorithm>
#include <ctype.h>
#include <stack>
#include <VersionHelpers.h>
#include <stdint.h>
#include <tchar.h>
#include <tlhelp32.h>
#include <time.h>
#include <WinUser.h>
#include <stdio.h>
#include <stdlib.h>
#include <Psapi.h>
#include <memory>
#include <TimeAPI.h>
#include <sstream>
#include <fstream>
#include <cstdint>
#include <iomanip>

#define MAX_CLASSNAME 255
#define MAX_WNDNAME MAX_CLASSNAME

#define INRANGE(x,a,b)		(x >= a && x <= b) 
#define GET_BITS( x )		(INRANGE(x,'0','9') ? (x - '0') : ((x&(~0x20)) - 'A' + 0xa))
#define GET_BYTE( x )		(GET_BITS(x[0]) << 4 | GET_BITS(x[1]))

std::string Windows7 = "C:\\Users\\user\\Desktop\\SACDriverModuleThread\\x64\\Release\\SACDriverModuleThread.sys";
std::string Windows10 = "c:\\username\\documents\\test64.sys";

typedef struct _KERNEL_PROCESSIDS_REQUEST
{
	ULONG CSGO;
	ULONG USERMODEPROGRAM;

	ULONG CSRSS;
	ULONG CSRSS2;

} KERNEL_PROCESSIDS_REQUEST, *PKERNEL_PROCESSIDS_REQUEST;

typedef struct _KERNEL_TERMINATION_REQUEST
{
	ULONG TERMINATIONPROCESSID;

} KERNEL_TERMINATION_REQUEST, *PKERNEL_TERMINATION_REQUEST;

typedef struct _KERNEL_THREADIDS_REQUEST
{
	ULONG THREAD1;
	ULONG THREAD2;
	ULONG THREAD3;
	ULONG THREAD4;

} KERNEL_THREADIDS_REQUEST, *PKERNEL_THREADIDS_REQUEST;

struct OverlayFinderParams {
	DWORD pidOwner = NULL;
	std::wstring wndClassName = L"";
	std::wstring wndName = L"";
	RECT pos = { 0, 0, 0, 0 }; // GetSystemMetrics with SM_CXSCREEN and SM_CYSCREEN can be useful here
	POINT res = { 0, 0 };
	float percentAllScreens = 0.0f;
	float percentMainScreen = 0.0f;
	DWORD style = NULL;
	DWORD styleEx = NULL;
	bool satisfyAllCriteria = false;
	std::vector<HWND> hwnds;
};

#define IO_PROGRAM_PROCESSID CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0700 , METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

#define IO_TERMINATION_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0701 , METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

#define IO_THREADIDS_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0701 , METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

HANDLE hDriver;
bool SendProcessIDs(ULONG CSGO, ULONG USERMODEPROGRAM, ULONG CSRSS, ULONG CSRSS2)
{
	if (hDriver == INVALID_HANDLE_VALUE)
		return false;

	DWORD Return, Bytes;
	KERNEL_PROCESSIDS_REQUEST ReadRequest;

	ReadRequest.CSGO = CSGO;
	ReadRequest.USERMODEPROGRAM = USERMODEPROGRAM;
	ReadRequest.CSRSS = CSRSS;
	ReadRequest.CSRSS2 = CSRSS2;

	// send code to our driver with the arguments
	if (DeviceIoControl(hDriver, IO_PROGRAM_PROCESSID, &ReadRequest,
		sizeof(ReadRequest), &ReadRequest, sizeof(ReadRequest), &Bytes, NULL))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool SendThreadIDs(ULONG Thread1, ULONG Thread2,ULONG Thread3, ULONG Thread4)
{
	if (hDriver == INVALID_HANDLE_VALUE)
		return false;

	DWORD Return, Bytes;
	KERNEL_THREADIDS_REQUEST ReadRequest;

	ReadRequest.THREAD1 = Thread1;
	ReadRequest.THREAD2 = Thread2;
	ReadRequest.THREAD3 = Thread3;
	ReadRequest.THREAD4 = Thread4;

	// send code to our driver with the arguments
	if (DeviceIoControl(hDriver, IO_THREADIDS_REQUEST, &ReadRequest,
		sizeof(ReadRequest), &ReadRequest, sizeof(ReadRequest), &Bytes, NULL))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Terminate(ULONG Terminate)
{
	if (hDriver == INVALID_HANDLE_VALUE)
		return false;

	DWORD Return, Bytes;
	KERNEL_TERMINATION_REQUEST ReadRequest;

	ReadRequest.TERMINATIONPROCESSID = Terminate;

	// send code to our driver with the arguments
	if (DeviceIoControl(hDriver, IO_TERMINATION_REQUEST, &ReadRequest,
		sizeof(ReadRequest), &ReadRequest, sizeof(ReadRequest), &Bytes, NULL))
	{
		return true;
	}
	else
	{
		return false;
	}
}

BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam)
{
	OverlayFinderParams& params = *(OverlayFinderParams*)lParam;

	unsigned char satisfiedCriteria = 0, unSatisfiedCriteria = 0;

	// If looking for windows of a specific PDI
	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	if (params.pidOwner != NULL)
		if (params.pidOwner == pid)
			++satisfiedCriteria; // Doesn't belong to the process targeted
		else
			++unSatisfiedCriteria;

	// If looking for windows of a specific class
	wchar_t className[MAX_CLASSNAME] = L"";
	GetClassName(hwnd, (LPSTR)className, MAX_CLASSNAME);
	std::wstring classNameWstr = className;
	if (params.wndClassName != L"")
		if (params.wndClassName == classNameWstr)
			++satisfiedCriteria; // Not the class targeted
		else
			++unSatisfiedCriteria;
	// If looking for windows with a specific name
	wchar_t windowName[MAX_WNDNAME] = L"";
	GetWindowText(hwnd, (LPSTR)windowName, MAX_CLASSNAME);
	std::wstring windowNameWstr = windowName;
	if (params.wndName != L"")
		if (params.wndName == windowNameWstr)
			++satisfiedCriteria; // Not the class targeted
		else
			++unSatisfiedCriteria;

	// If looking for window at a specific position
	RECT pos;
	GetWindowRect(hwnd, &pos);
	if (params.pos.left || params.pos.top || params.pos.right || params.pos.bottom)
		if (params.pos.left == pos.left && params.pos.top == pos.top && params.pos.right == pos.right && params.pos.bottom == pos.bottom)
			++satisfiedCriteria;
		else
			++unSatisfiedCriteria;

	// If looking for window of a specific size
	POINT res = { pos.right - pos.left, pos.bottom - pos.top };
	if (params.res.x || params.res.y)
		if (res.x == params.res.x && res.y == params.res.y)
			++satisfiedCriteria;
		else
			++unSatisfiedCriteria;

	// If looking for windows taking more than a specific percentage of all the screens
	float ratioAllScreensX = res.x / GetSystemMetrics(SM_CXSCREEN);
	float ratioAllScreensY = res.y / GetSystemMetrics(SM_CYSCREEN);
	float percentAllScreens = ratioAllScreensX * ratioAllScreensY * 100;
	if (params.percentAllScreens != 0.0f)
		if (percentAllScreens >= params.percentAllScreens)
			++satisfiedCriteria;
		else
			++unSatisfiedCriteria;
	// If looking for windows taking more than a specific percentage or the main screen
	RECT desktopRect;
	GetWindowRect(GetDesktopWindow(), &desktopRect);
	POINT desktopRes = { desktopRect.right - desktopRect.left, desktopRect.bottom - desktopRect.top };
	float ratioMainScreenX = res.x / desktopRes.x;
	float ratioMainScreenY = res.y / desktopRes.y;
	float percentMainScreen = ratioMainScreenX * ratioMainScreenY * 100;
	if (params.percentMainScreen != 0.0f)
		if (percentAllScreens >= params.percentMainScreen)
			++satisfiedCriteria;
		else
			++unSatisfiedCriteria;

	// Looking for windows with specific styles
	LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
	if (params.style)
		if (params.style & style)
			++satisfiedCriteria;
		else
			++unSatisfiedCriteria;

	// Looking for windows with specific extended styles
	LONG_PTR styleEx = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
	if (params.styleEx)
		if (params.styleEx & styleEx)
			++satisfiedCriteria;
		else
			++unSatisfiedCriteria;

	if (!satisfiedCriteria)
		return TRUE;

	if (params.satisfyAllCriteria && unSatisfiedCriteria)
		return TRUE;

	// If looking for multiple windows
	params.hwnds.push_back(hwnd);
	return TRUE;
}

std::vector<HWND> OverlayFinder(OverlayFinderParams params)
{
	EnumWindows(EnumWindowsCallback, (LPARAM)&params);
	return params.hwnds;
}

inline bool is_match(const uint8_t* addr, const uint8_t* pat, const uint8_t* msk)
{
	size_t n = 0;
	while (addr[n] == pat[n] || msk[n] == (uint8_t)'?') {
		if (!msk[++n]) {
			return true;
		}
	}
	return false;
}

inline uint8_t* find_pattern(uint8_t* range_start, const uint32_t len, const char* pattern)
{
	size_t l = strlen(pattern);
	uint8_t* patt_base = static_cast<uint8_t*>(_alloca(l >> 1));
	uint8_t* msk_base = static_cast<uint8_t*>(_alloca(l >> 1));
	uint8_t* pat = patt_base;
	uint8_t* msk = msk_base;
	l = 0;
	while (*pattern) {
		if (*(uint8_t*)pattern == (uint8_t)'\?') {
			*pat++ = 0;
			*msk++ = '?';
			pattern += ((*(uint16_t*)pattern == (uint16_t)'\?\?') ? 3 : 2);
		}
		else {
			*pat++ = GET_BYTE(pattern);
			*msk++ = 'x';
			pattern += 3;
		}
		l++;
	}
	*msk = 0;
	pat = patt_base;
	msk = msk_base;
	for (uint32_t n = 0; n < (len - l); ++n)
	{
		if (is_match(range_start + n, patt_base, msk_base)) {
			return range_start + n;
		}
	}
	return nullptr;
}

inline void open_binary_file(const std::string & file, std::vector<uint8_t>& data)
{
	std::ifstream file_stream(file, std::ios::binary);
	file_stream.unsetf(std::ios::skipws);
	file_stream.seekg(0, std::ios::end);

	const auto file_size = file_stream.tellg();

	file_stream.seekg(0, std::ios::beg);
	data.reserve(static_cast<uint32_t>(file_size));
	data.insert(data.begin(), std::istream_iterator<uint8_t>(file_stream), std::istream_iterator<uint8_t>());
}

const std::vector<std::pair<std::string, const char*>> ModuleSignature = {
	{ "chams material",				"89 47 10 8B C7 83 FA 10 72 02 8B 07 53 FF 75 08 03 F0" },
	{ "interfaces loaded",			"49 6E 74 65 72 66 61 63 65 73 20 4C 6F 61 64 65 64 00" },
	{ "xml shit",					"8B C2 EB F9 E8 33 0B 00 00 85 C0 75 03 32 C0 C3 64 A1 18 00 00 00 56" },
	{ "reflective loader",			"52 65 66 6C 65 63 74 69 76 65 4C 6F 61 64" },
	{ "chams shit",					"22 25 73 22 09 09 0A 7B 09 09 0A 09 22 24 62 61 73 65 74 65 78 74 75 72 65 22 20 22 76 67 75" },
	{ "bomb esp",					"42 6F 6D 62 3A 20 25 2E 31 66 00" },
	{ "perfecthook entry point",	"55 8B EC E8 ? ? ? ? 8B 0D ? ? ? ? E8" }
};

const std::vector<std::pair<std::string, const char*>> ExternalSignature = {
	{ "chams material",				"89 47 10 8B C7 83 FA 10 72 02 8B 07 53 FF 75 08 03 F0" },
	{ "interfaces loaded",			"49 6E 74 65 72 66 61 63 65 73 20 4C 6F 61 64 65 64 00" },
	{ "xml shit",					"58 4D 4C 5F 53 55 43 43 45" },
	{ "reflective loader",			"52 65 66 6C 65 63 74 69 76 65 4C 6F 61 64" },
	{ "chams shit",					"22 25 73 22 09 09 0A 7B 09 09 0A 09 22 24 62 61 73 65 74 65 78 74 75 72 65 22 20 22 76 67 75" },
	{ "bomb esp",					"42 6F 6D 62 3A 20 25 2E 31 66 00" },
	{ "perfecthook entry point",	"55 8B EC E8 ? ? ? ? 8B 0D ? ? ? ? E8" }
};

void ClasseWindow(LPCSTR WindowClasse)
{
	HWND WinClasse = FindWindowExA(NULL, NULL, WindowClasse, NULL);
	if (WinClasse > 0)
	{
		SendMessage(WinClasse, WM_CLOSE, 0, 0);

	}
}

bool TitleWindow(LPCSTR WindowTitle)
{
	HWND WinTitle = FindWindowA(NULL, WindowTitle);
	if (WinTitle > 0) {
		SendMessage(WinTitle, WM_CLOSE, 0, 0);  //CLOSE HACK WINTITLE
		return false;
	}
	return true;
}

void GetProcId(char* ProcName)
{
	PROCESSENTRY32 pe32;
	HANDLE hSnapshot = NULL;

	pe32.dwSize = sizeof(PROCESSENTRY32);
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (Process32First(hSnapshot, &pe32))
	{
		do {
			if (strcmp(pe32.szExeFile, ProcName) == 0)
			{
				HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);  // Close detected process
				Terminate(pe32.th32ProcessID);
				TerminateProcess(hProcess, NULL);
				CloseHandle(hProcess);
			}
		} while (Process32Next(hSnapshot, &pe32));
	}
	if (hSnapshot != INVALID_HANDLE_VALUE)
		CloseHandle(hSnapshot);
}

inline bool HideThread()
{
	typedef NTSTATUS(NTAPI *pNtSetInformationThread)
		(HANDLE, UINT, PVOID, ULONG);
	NTSTATUS Status;

	// Get NtSetInformationThread
	pNtSetInformationThread NtSIT = (pNtSetInformationThread)
		GetProcAddress(GetModuleHandle(TEXT("ntdll.dll")),
			"NtSetInformationThread");

	// Shouldn't fail
	if (NtSIT == NULL)
		return false;

	Status = NtSIT(GetCurrentThread(),
		0x11, // HideThreadFromDebugger
		0, 0);

	if (Status != 0x00000000)
		return false;
	else
		return true;
}

DWORD ModuleScanningID;
DWORD ExternlScanningID;
DWORD ExternalCheckingID;
DWORD ThreadCheckingID;
DWORD CSRSS;
DWORD CSRSS2;

HANDLE ModuleScanningHandle;
HANDLE ExternlScanningHandle;
HANDLE ExternalCheckingHandle;
HANDLE ThreadCheckingHandle;

ULONG CSGOProcessID;
HANDLE CSGOHandle;



bool ThreadQuit = false;

const std::string currentDateTime()
{
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

	return buf;
}

void ErrorLogs(std::string Logs)
{
	using namespace std;
	ofstream myfile;
	myfile.open("SAC - LogReport.txt");
	myfile << "\n" << "[" << currentDateTime() << "]" << Logs << "\n";
	myfile.close();
}

DWORD GetProcID(char* ProcName)
{
	PROCESSENTRY32   pe32;
	HANDLE         hSnapshot = NULL;

	pe32.dwSize = sizeof(PROCESSENTRY32);
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (Process32First(hSnapshot, &pe32))
	{
		do {
			if (strcmp(pe32.szExeFile, ProcName) == 0)
				break;
		} while (Process32Next(hSnapshot, &pe32));
	}

	if (hSnapshot != INVALID_HANDLE_VALUE)
		CloseHandle(hSnapshot);

	return pe32.th32ProcessID;
}

DWORD WINAPI ModuleScanning(LPVOID lpParameter)
{
	while (!ThreadQuit)
	{
		HideThread();
		std::vector<uint8_t> data;

		HMODULE hMods[1024];
		HANDLE hProcess;
		DWORD cbNeeded;
		unsigned int i;


		// Get a handle to the process.

		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
			PROCESS_VM_READ,
			FALSE, GetProcID("csgo.exe"));

		if (NULL == hProcess)
			return 1;

		// Get a list of all the modules in this process.

		if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
		{
			for (i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
			{
				TCHAR szModName[MAX_PATH];

				// Get the full path to the module's file.

				if (GetModuleFileNameEx(hProcess, hMods[i], szModName,
					sizeof(szModName) / sizeof(TCHAR)))
				{
					// Print the module name and handle value.a

					open_binary_file(szModName, data);



					auto hack_features = 0;

					for (auto & entry : ModuleSignature)
					{
						const auto offset = find_pattern(data.data(), data.size(), entry.second);
						if (offset != nullptr)
						{
						
							hack_features++;
						}
					}


					if (hack_features)
					{
						ErrorLogs("Blacklisted Module Found");
						ThreadQuit = true;
					}

			
				}
				Sleep(300);
			}
			Sleep(300);
		}

		// Release the handle to the process.

		CloseHandle(hProcess);
	}
	ThreadQuit = true;
	return false;
}

DWORD WINAPI ExternlScanning(LPVOID lpParameter)
{
	while (!ThreadQuit)
	{
		HideThread();
		DWORD aProcesses[1024], cbNeeded, cProcesses;
		unsigned int i;

		if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
		{
			ThreadQuit = true;
		}


		// Calculate how many process identifiers were returned.

		cProcesses = cbNeeded / sizeof(DWORD);

		// Print the name and process identifier for each process.

		for (i = 0; i < cProcesses; i++)
		{
			if (aProcesses[i] != 0)
			{
				TCHAR szProcessName[MAX_PATH] = TEXT("");
				// Get a handle to the process.

				HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS,
					FALSE, aProcesses[i]);

				// Get the process name.
				LPSTR FilePath = "";
				HMODULE hMod = NULL;
				DWORD cbNeeded;
				if (NULL != hProcess)
				{


					if (EnumProcessModules(hProcess, &hMod, sizeof(hMod),
						&cbNeeded))
					{

						GetModuleBaseName(hProcess, hMod, szProcessName,
							sizeof(szProcessName) / sizeof(TCHAR));

						//GetProcessImageFileNameA(hProcess, FilePath, MAX_PATH);

						//QueryFullProcessImageName(hProcess, PROCESS_NAME_NATIVE, FilePath, 0);

						//std::cout << "FilePath test: " << FilePath << std::endl;
						//std::cout << "FileName test: " << szProcessName << std::endl;
						Sleep(300);
					} 

				}
				Sleep(300);
				TCHAR filename[MAX_PATH];
				HWND WinClasse;
				LPCSTR Buffer = NULL;
				

				CloseHandle(hProcess);
			}
		}

		Sleep(300);
	}
	return false;
}

DWORD WINAPI ExternalChecking(LPVOID lpParameter)
{
	while (!ThreadQuit)
	{
		HideThread();
		GetProcId("ahk.exe");
		GetProcId("ida.exe");
		GetProcId("ollydbg.exe*32");
		GetProcId("ollydbg.exe");
		GetProcId("bvkhex.exe");
		//GetProcId("cheatengine-x86_64.exe");
		GetProcId("HxD.exe");
		GetProcId("procexp2.exe");
		GetProcId("Hide Toolz3.3.3.exe");
		GetProcId("SbieSvc.exe");   
		GetProcId("SbieSvc*32.exe"); 
		GetProcId("SbieSvc*32.exe"); 
		GetProcId("SbieCtrl.exe");
		Sleep(400);
		TitleWindow("!xSpeed 6.0");
		TitleWindow("!xSpeed 6.0");
		TitleWindow("!xSpeed.net 2");
		TitleWindow("!xSpeed.net 3");
		TitleWindow("!xSpeed.net 6");
		TitleWindow("!xSpeed.net");
		TitleWindow("!xSpeedPro");
		TitleWindow("!xpeed.net 1.41");
		TitleWindow("99QJ MU Bot");
		TitleWindow("AE Bot v1.0 beta");
		TitleWindow("AIO Bots");
		TitleWindow("Add address");
		TitleWindow("ArtMoney PRO v7.27");
		TitleWindow("ArtMoney SE v7.31");
		TitleWindow("ArtMoney SE v7.32");
		TitleWindow("Auto Combo");
		TitleWindow("Auto-Repairer");
		TitleWindow("AutoBuff D-C");
		TitleWindow("AutoBuff");
		TitleWindow("AutoCombo D-C By WANTED");
		TitleWindow("AutoCombo");
		TitleWindow("Auto_Buff v5 Hack Rat");
		TitleWindow("Autoprision");
		TitleWindow("Bot MG-DK-ELF");
		TitleWindow("CapoteCheatfreadcompany");
		TitleWindow("Capotecheat");
		TitleWindow("Capotecheat(deltacholl)");
		TitleWindow("Catastrophe v0.1");
		TitleWindow("Catastrophe v1.2");
		TitleWindow("Catastrophe");
		TitleWindow("Chaos Bot 2.1.0");
		TitleWindow("CharBlaster");
		TitleWindow("CharEditor (www.darkhacker.com.ar)");
		Sleep(400);
		//TitleWindow("Cheat Engine 5.0");
		//TitleWindow("Cheat Engine 5.1");
		//TitleWindow("Cheat Engine 5.1.1");
		//TitleWindow("Cheat Engine 5.2");
		//TitleWindow("Cheat Engine 5.3");
		//TitleWindow("Cheat Engine 5.4");
		//TitleWindow("Cheat Engine 5.5");
		//TitleWindow("Cheat Engine 5.6");
		//TitleWindow("Cheat Engine 5.6.1");
		//TitleWindow("Cheat Engine 6.0");
		//TitleWindow("Cheat Engine 6.1");
		//TitleWindow("Cheat Engine 6.4");
		//TitleWindow("Cheat Engine");
		TitleWindow("Cheat Happens v3.95b1/b2");
		TitleWindow("Cheat Happens v3.95b3");
		TitleWindow("Cheat Happens v3.96b2");
		TitleWindow("Cheat Happens v3.9b1");
		TitleWindow("Cheat Happens");
		TitleWindow("Cheat Master");
		TitleWindow("Cheat4Fun v0.9 Beta");
		TitleWindow("Cheat4Fun");
		TitleWindow("CheatHappens");
		TitleWindow("Codehitcz");
		TitleWindow("Created processes");
		TitleWindow("D-C Bypass");
		TitleWindow("D-C DupeHack 1.0");
		TitleWindow("D-C Master Inject v1.0 by WANTED");
		TitleWindow("DC Mu 1.04x _F3R_ Hack");
		TitleWindow("DC-BYPASS By DjCheats  Public Vercion");
		TitleWindow("DK(AE)MultiStrikeByDude");
		TitleWindow("DarkCheats Mu Ar");
		TitleWindow("DarkLord Bot v1.1");
		TitleWindow("DarkyStats (www.darkhacker.com.ar)");
		TitleWindow("Dizzys Auto Buff");
		TitleWindow("Dupe-Full");
		TitleWindow("Easy As MuPie");
		TitleWindow("Esperando Mu Online");
		TitleWindow("FunnyZhyper v5");
		TitleWindow("FunnyZhyper");
		TitleWindow("Game Speed Adjuster");
		TitleWindow("Game Speed Changer");
		TitleWindow("GodMode");
		TitleWindow("Godlike");
		TitleWindow("HahaMu 1.16");
		TitleWindow("Hasty MU 0.3");
		TitleWindow("Hasty MU");
		TitleWindow("HastyMU");
		TitleWindow("HastyMu 1.1.0 NEW");
		TitleWindow("HastyMu v0.1");
		TitleWindow("HastyMu v0.2");
		TitleWindow("HastyMu v0.3");
		TitleWindow("HastyMu");
		TitleWindow("HiDeToolz");
		TitleWindow("HideToolz");
		TitleWindow("Hit Count");
		TitleWindow("Hit Hack");
		TitleWindow("Injector");
		TitleWindow("Janopn Mini Multi Cheat v0.1");
		TitleWindow("Jewel Drop Beta");
		TitleWindow("JoyToKey");
		TitleWindow("Kill");
		TitleWindow("Lipsum v1 and v2");
		TitleWindow("Lipsum_v2");
		TitleWindow("Load File");
		TitleWindow("MJB Perfect DL Bot");
		Sleep(400);
		TitleWindow("MLEngine");
		TitleWindow("MU Lite Trainer");
		TitleWindow("MU Utilidades");
		TitleWindow("MU-SS4 Speed Hack 1.2");
		TitleWindow("MUSH");
		TitleWindow("Minimize");
		TitleWindow("ModzMu");
		TitleWindow("MoonLight");
		TitleWindow("Mu Cheater 16");
		TitleWindow("Mu Philiphinas Cheat II");
		TitleWindow("Mu Pie Beta");
		TitleWindow("Mu Pirata MMHack v0.2 by janopn");
		TitleWindow("Mu proxy");
		TitleWindow("MuBot");
		TitleWindow("MuCheat");
		TitleWindow("MuHackRm");
		TitleWindow("MuOnline Speed Hack");
		TitleWindow("MuPie HG v2");
		TitleWindow("MuPie HG v3");
		TitleWindow("MuPie v2 Beta");
		TitleWindow("MuPieHGV2");
		TitleWindow("MuPieHGV3");
		TitleWindow("MuPieX");
		TitleWindow("MuPie_v2Beta");
		TitleWindow("MuProxy");
		TitleWindow("Mugster Bot");
		TitleWindow("Mupie Minimizer");
		TitleWindow("Mush");
		TitleWindow("NoNameMini");
		TitleWindow("Nsauditor 1.9.1");
		TitleWindow("Olly Debugger");
		TitleWindow("Overclock Menu");
		TitleWindow("Perfect AutoPotion");
		TitleWindow("Permit");
		TitleWindow("PeruCheats");
		TitleWindow("ProxCheatsX 2.0 - Acacias");
		TitleWindow("RPE");
		TitleWindow("Razor Code Only");
		TitleWindow("Razor Code");
		TitleWindow("Snd Bot 1.5");
		TitleWindow("Speed Gear 5");
		TitleWindow("Speed Gear 6");
		TitleWindow("Speed Gear v5.00");
		TitleWindow("Speed Gear");
		TitleWindow("Speed Hack 99.62t");
		TitleWindow("Speed Hack Simplifier 1.0-1.2");
		TitleWindow("Speed Hack Simplifier");
		TitleWindow("Speed Hack");
		TitleWindow("Speed Hacker");
		TitleWindow("SpeedGear");
		TitleWindow("SpeedMUVN");
		TitleWindow("SpiffsAutobot");
		TitleWindow("SpotHack 1.1");
		Sleep(400);
		TitleWindow("SpotHack");
		TitleWindow("Stop");
		TitleWindow("Super Bot");
		TitleWindow("T Search");
		TitleWindow("Tablet 2");
		TitleWindow("The following opcodes accessed the selected address");
		TitleWindow("Trade HACK 1.8");
		TitleWindow("Ultimate Cheat");
		TitleWindow("UoPilot  v2.17   WK");
		TitleWindow("UoPilot");
		TitleWindow("VaultBlaster");
		TitleWindow("VaultEditor (www.darkhacker.com.ar)");
		TitleWindow("WPE PRO");
		TitleWindow("WPePro 0.9a");
		TitleWindow("WPePro 1.3");
		TitleWindow("Wall");
		TitleWindow("WildProxy 1.0 Alpha");
		TitleWindow("WildProxy v0.1");
		TitleWindow("WildProxy v0.2");
		TitleWindow("WildProxy v0.3");
		TitleWindow("WildProxy v1.0 Public");
		TitleWindow("WildProxy");
		TitleWindow("Xelerator 1.4");
		TitleWindow("Xelerator 2.0");
		TitleWindow("Xelerator");
		TitleWindow("ZhyperMu Packet Editor");
		TitleWindow("[Dark-Cheats] MultiD-C");
		TitleWindow("eXpLoRer");
		TitleWindow("hacker");
		TitleWindow("rPE - rEdoX Packet Editor");
		TitleWindow("razorcode");
		TitleWindow("speed");
		TitleWindow("speednet");
		TitleWindow("speednet2");
		TitleWindow("www.55xp.com");
		TitleWindow("xSpeed.net 3.0");
		TitleWindow("BVKHEX");
		TitleWindow("OllyDbg");
		TitleWindow("HxD");
		TitleWindow("BY DARKTERRO");
		TitleWindow("Tim Geimi Jaks - DarkTerro");
		Sleep(400);
		ClasseWindow("ProcessHacker");
		ClasseWindow("PhTreeNew");
		ClasseWindow("RegEdit_RegEdit");
		ClasseWindow("0x150114 (1376532)");
		ClasseWindow("SysListView32");
		ClasseWindow("Tmb");
		ClasseWindow("TformSettings");
		ClasseWindow("Afx:400000:8:10011:0:20575");
		ClasseWindow("TWildProxyMain");
		ClasseWindow("TUserdefinedform");
		ClasseWindow("TformAddressChange");
		ClasseWindow("TMemoryBrowser");
		ClasseWindow("TFoundCodeDialog");
		Sleep(400);
	}
	return false;
}

DWORD WINAPI ThreadChecking(LPVOID lpParameter)
{
	while (!ThreadQuit)
	{
		HideThread();
		Sleep(100);
	}
	return false;
}

BOOL LoadDriver(std::string TargetDriver, std::string TargetServiceName, std::string TargetServiceDesc)
{
	using namespace std;
	SC_HANDLE ServiceManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (!ServiceManager) return FALSE;
	SC_HANDLE ServiceHandle = CreateService(ServiceManager, TargetServiceName.c_str(), TargetServiceDesc.c_str(), SERVICE_START | DELETE | SERVICE_STOP, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE, TargetDriver.c_str(), NULL, NULL, NULL, NULL, NULL);
	if (!ServiceHandle)
	{
		ServiceHandle = OpenService(ServiceManager, TargetServiceName.c_str(), SERVICE_START | DELETE | SERVICE_STOP);
		if (!ServiceHandle) return FALSE;
	}
	if (!StartServiceA(ServiceHandle, NULL, NULL)) return FALSE;
	CloseServiceHandle(ServiceHandle);
	CloseServiceHandle(ServiceManager);
	return TRUE;
}

BOOL UnloadDriver(std::string TargetServiceName)
{
	using namespace std;
	SERVICE_STATUS ServiceStatus;
	SC_HANDLE ServiceManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (!ServiceManager) return FALSE;
	SC_HANDLE ServiceHandle = OpenService(ServiceManager, TargetServiceName.c_str(), SERVICE_STOP | DELETE);
	if (!ServiceHandle) return FALSE;
	if (!ControlService(ServiceHandle, SERVICE_CONTROL_STOP, &ServiceStatus)) return FALSE;
	if (!DeleteService(ServiceHandle)) return FALSE;
	CloseServiceHandle(ServiceHandle);
	CloseServiceHandle(ServiceManager);
	return TRUE;
}

void OnExit()
{
	ThreadQuit = true;
	UnloadDriver("SACDriverModuleThread");
	CloseHandle(hDriver);
	CloseHandle(ModuleScanningHandle);
	CloseHandle(ExternlScanningHandle);
	CloseHandle(ExternalCheckingHandle);
	CloseHandle(ThreadCheckingHandle);
	CloseHandle(CSGOHandle);

	ErrorLogs("Exit Called!");
}

#define MAX_PROCESSES 1024 
DWORD FindProcessId(__in_z LPCTSTR lpcszFileName)
{
	LPDWORD lpdwProcessIds;
	LPTSTR  lpszBaseName;
	HANDLE  hProcess;
	DWORD   i, cdwProcesses, dwProcessId = 0;

	lpdwProcessIds = (LPDWORD)HeapAlloc(GetProcessHeap(), 0, MAX_PROCESSES * sizeof(DWORD));
	if (lpdwProcessIds != NULL)
	{
		if (EnumProcesses(lpdwProcessIds, MAX_PROCESSES * sizeof(DWORD), &cdwProcesses))
		{
			lpszBaseName = (LPTSTR)HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(TCHAR));
			if (lpszBaseName != NULL)
			{
				cdwProcesses /= sizeof(DWORD);
				for (i = 0; i < cdwProcesses; i++)
				{
					hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, lpdwProcessIds[i]);
					if (hProcess != NULL)
					{
						if (GetModuleBaseName(hProcess, NULL, lpszBaseName, MAX_PATH) > 0)
						{
							if (!lstrcmpi(lpszBaseName, lpcszFileName))
							{
								dwProcessId = lpdwProcessIds[i];
								CloseHandle(hProcess);
								break;
							}
						}
						CloseHandle(hProcess);
					}
				}
				HeapFree(GetProcessHeap(), 0, (LPVOID)lpszBaseName);
			}
		}
		HeapFree(GetProcessHeap(), 0, (LPVOID)lpdwProcessIds);
	}
	return dwProcessId;
}

void OpenCSGO()
{
	STARTUPINFO si = { sizeof(STARTUPINFO) };
	PROCESS_INFORMATION pi;
	CreateProcess("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Counter-Strike Global Offensive\\csgo.exe",
		0, 0, 0, 0, 0, 0, 0, &si, &pi);
}

std::vector<DWORD> GetPIDs(std::wstring targetProcessName)
{
	using namespace std;
	vector<DWORD> pids;
	if (targetProcessName == L"")
		return pids;
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32W entry;
	entry.dwSize = sizeof entry;
	if (!Process32FirstW(snap, &entry))
		return pids;
	do {
		if (wstring(entry.szExeFile) == targetProcessName) {
			pids.emplace_back(entry.th32ProcessID);
		}
	} while (Process32NextW(snap, &entry));
	return pids;
}



bool Quit = false;
int main()
{
	using namespace std;
	if (!UnloadDriver("SACDriverModuleThread"))
	{
		ErrorLogs("Driver Not Found \ Driver Not UnLoaded!");
	}

	if (!LoadDriver(Windows7, "SACDriverModuleThread", "SAC - Sagaan's Anti Cheat"))
	{
		ErrorLogs("Driver Not Found \ Driver Not Loaded!");
	}

	hDriver = CreateFileA("\\\\.\\SACDriverModuleThread", GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);

	if (!hDriver)
	{
		ErrorLogs("Driver's Register File Not Found");
	}

	CSGOProcessID = FindProcessId("csgo.exe");

	CSGOHandle = OpenProcess(PROCESS_QUERY_INFORMATION |
		PROCESS_VM_READ, FALSE, CSGOProcessID);

	

	wstring we1 = L"";
	wstring lsassNoStr1 = we1 + L'c' + L's' + L'r' + L's' + L's' + L'.' + L'e' + L'x' + L'e';
	vector<DWORD> CSRSSPID = GetPIDs(lsassNoStr1);


	if (CSRSSPID.empty())
	{
		ErrorLogs("CSRSS Not Found");
	}
	sort(CSRSSPID.begin(), CSRSSPID.end()); // In case there is several lsass.exe running (?) take the first one (based on PID)

	CSRSS = CSRSSPID[0];
	CSRSS2 = CSRSSPID[1];

	if (!SendProcessIDs(CSGOProcessID, GetCurrentProcessId(), CSRSS, CSRSS2))
	{
		ErrorLogs("Error Sending ProcessID To Driver");
	}

	ModuleScanningHandle = CreateThread(0, 0, ModuleScanning, 0, 0, &ModuleScanningID);
	ExternlScanningHandle = CreateThread(0, 0, ExternlScanning, 0, 0, &ExternlScanningID);
	ExternalCheckingHandle = CreateThread(0, 0, ExternalChecking, 0, 0, &ExternalCheckingID);
	ThreadCheckingHandle = CreateThread(0, 0, ThreadChecking, 0, 0, &ThreadCheckingID);

	if (!SendThreadIDs(ModuleScanningID, ExternlScanningID, ExternalCheckingID, ThreadCheckingID))
	{
		ErrorLogs("Error Sending Threads To Driver");
	}

	while (!Quit)
	{
		atexit(OnExit);
		if (ThreadQuit)
		{
			UnloadDriver("SACDriverModuleThread");
			CloseHandle(hDriver);
			CloseHandle(ModuleScanningHandle);
			CloseHandle(ExternlScanningHandle);
			CloseHandle(ExternalCheckingHandle);
			CloseHandle(ThreadCheckingHandle);
			CloseHandle(CSGOHandle);
			
			ErrorLogs("Exit Called!");
			exit(1);
		}

		OverlayFinderParams params;
		params.style = WS_VISIBLE;
		params.styleEx = WS_EX_LAYERED | WS_EX_TRANSPARENT;
		params.percentMainScreen = 90.0f;
		params.satisfyAllCriteria = true;
		std::vector<HWND> hwnds = OverlayFinder(params);

		for (int i(0); i < hwnds.size(); ++i) {
			DWORD pid = 0;
			DWORD tid = GetWindowThreadProcessId(hwnds[i], &pid);
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
			if (hProcess)
			{
				char cheatPath[MAX_PATH] = "";
				GetProcessImageFileNameA(hProcess, (LPSTR)&cheatPath, MAX_PATH);
			}
			Terminate(pid);
			CloseHandle(hProcess);
		}
		Sleep(400);
	}

    return 0;
}

