#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <string>
#include <sstream>
#include <winbase.h>
#include <commdlg.h>
#include <tchar.h>
#include <commctrl.h>
#include <wingdi.h>
#include <iostream>
#include <shellapi.h> 
#include <cmath> // Для градиента. Не трогайте пожалуйста)

//  ОПРЕДЕЛЯЕМ ВСЕ ID ЗДЕСЬ 
#define IDI_MAIN_ICON 101
#define IDC_BTN_INJECT 1001
#define IDC_DLL_PATH 	1004
#define IDC_BTN_SELECT 1005
#define IDC_PROC_NAME 1007
#define IDC_BTN_ABOUT 1013
#define IDC_BTN_SETTINGS 1014 
#define IDC_BTN_CHECK_STATUS 1021
#define IDC_BTN_SELECT_DLL 1022
#define IDC_BTN_OPEN_FOLDER 1023
#define IDC_STATIC_PID_STATUS 1024 

// ID ДЛЯ ОКНА НАСТРОЕК
#define IDC_BTN_COLOR 2001
#define IDC_BTN_GRAD_TOP 2002
#define IDC_BTN_GRAD_BOTTOM 2003
#define IDC_BTN_SAVE_COLORS 2004
#define IDC_EDIT_NOTES 2006
#define IDC_BTN_HOTKEY 2008
#define IDC_STATIC_HOTKEY_DISPLAY 2009
#define IDC_LIST_THEMES 2010 


// ЛИНКОВКА БИБЛИОТЕК
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shell32.lib") 

// ГЛОБАЛЬНЫЕ ИДЕНТИФИКАТОРЫ И ПЕРЕМЕННЫЕ

const wchar_t g_szClassName[] = L"FediskaInjectorClass";
HINSTANCE g_hInst;

HFONT g_hFont;

HICON g_hGameIcon = NULL;
const wchar_t g_szDefaultTitle[] = L"Fediska Injector v5.6";
const wchar_t g_szMaskedTitle[] = L"Windows Task Manager";

HBRUSH g_hDarkBrush = NULL;

// ЦВЕТОВЫЕ ПЕРЕМЕННЫЕ
COLORREF g_crTextColor = RGB(255, 255, 255);
COLORREF g_crGradTop = RGB(64, 0, 0);
COLORREF g_crGradBottom = RGB(30, 30, 30);
COLORREF g_ccCustomColors[16] = { 0 };

// 🔥 НОВЫЕ ПЕРЕМЕННЫЕ ДЛЯ АНИМАЦИИ ГРАДИЕНТА (1 = вверх, -1 = вниз)
int g_dirR = 1;
int g_dirG = 1;
int g_dirB = 1;
int g_dirR2 = 1;
int g_dirG2 = 1;
int g_dirB2 = 1;

// ТАЙМЕРЫ
HWND g_hwndHover = NULL;
#define TIMER_ID_HOVER 1337
#define TIMER_ID_STATUS 1338 
#define TIMER_ID_ANIMATION 1339 

// Хендлы элементов управления
HWND g_hwndDllPath = NULL;
HWND g_hwndProcName = NULL;
HWND g_hwndPidStatus = NULL;
// ХЕНДЛ ДЛЯ ОКНА НАСТРОЕК
HWND g_hwndNotes = NULL;
HWND g_hwndThemesList = NULL;

//  Буферы путей и заметок
wchar_t g_dllPath[MAX_PATH] = L"C:\\Users\\Public\\My_CSS_Hack.dll";
wchar_t g_procNameBuffer[MAX_PATH] = L"TargetProcess.exe";

#define MAX_NOTES_LENGTH 256
wchar_t g_notesBuffer[MAX_NOTES_LENGTH] = L"Insert your notes here...";

UINT g_hotKeyMod = MOD_CONTROL;
UINT g_hotKeyVk = VK_F9;
wchar_t g_hotKeyVkStr[10] = L"120"; // 120 = VK_F9

//НОВАЯ СТРУКТУРА ДЛЯ ТЕМ
typedef struct {
	const wchar_t* name;
	COLORREF textColor;
	COLORREF gradTop;
	COLORREF gradBottom;
} ThemePreset;

//НАШИ ГОТОВЫЕ ТЕМЫ
const ThemePreset g_themes[] = {
	{
		L"Classic Red & Black (Default)",
		RGB(255, 255, 255), // Белый текст
		RGB(64, 0, 0), // Темно-красный верх
		RGB(30, 30, 30) // Темно-серый низ
	},
	{
		L"Cyber Green (Matrix)",
		RGB(0, 255, 0), // Ярко-зеленый текст
		RGB(0, 64, 0), // Темно-зеленый верх
		RGB(10, 30, 10) // Очень темно-зеленый низ
	},
	{
		L"Black & Yellow (Khaki)",
		RGB(200, 200, 150), // Светло-желтый текст
		RGB(40, 40, 10), // Темный (верх)
		RGB(15, 15, 15) // Почти черный (низ)
	}
};

const size_t g_themeCount = sizeof(g_themes) / sizeof(ThemePreset);

//
// СТРУКТУРЫ И ПРОТОТИПЫ ДЛЯ NTAPI (НЕ МЕНЯТЬ ИНАЧЕ ВСЕ СЛОМАЕТЕ)
//

typedef LONG NTSTATUS;
typedef struct _UNICODE_STRING { USHORT Length; USHORT MaximumLength; PWSTR Buffer; } UNICODE_STRING, * PUNICODE_STRING;
typedef struct _CLIENT_ID { HANDLE UniqueProcess; HANDLE UniqueThread; } CLIENT_ID, * PCLIENT_ID;
typedef struct _OBJECT_ATTRIBUTES { ULONG Length; HANDLE RootDirectory; PWSTR ObjectName; ULONG Attributes; PVOID SecurityDescriptor; PVOID SecurityQualityOfService; } OBJECT_ATTRIBUTES, * POBJECT_ATTRIBUTES;

#define InitializeObjectAttributes( p, n, a, r, s ) { \
	(p)->Length = sizeof( OBJECT_ATTRIBUTES ); 			\
	(p)->RootDirectory = r; 							\
	(p)->Attributes = a; 								\
	(p)->ObjectName = n; 								\
	(p)->SecurityDescriptor = s; 						\
	(p)->SecurityQualityOfService = NULL; 				\
}

typedef NTSTATUS(WINAPI* PNTOPENPROCESS)(
	OUT PHANDLE ProcessHandle,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID ClientId OPTIONAL
	);

// ФУНКЦИИ
void ShowMessage(const wchar_t* title, const wchar_t* message, bool success);
bool CheckFileExists(const wchar_t* path);
DWORD GetProcessIdByName(const wchar_t* processName);
BOOL InjectDll(DWORD pid, const wchar_t* dllPath);
void CheckDllStatus();
void PerformInjection();
void LoadSettings();
void SaveSettings();
void ChooseTextColor(HWND hwnd);
void ChooseGradTopColor(HWND hwnd);
void ChooseGradBottomColor(HWND hwnd);

void UpdatePidStatus(HWND hwndMain);

//ПРОТОТИПЫ ДЛЯ ОКНА НАСТРОЕК
LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void OpenSettingsWindow(HWND hParent);


// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ (ОПРЕДЕЛЕНИЯ)

void ShowMessage(const wchar_t* title, const wchar_t* message, bool success) {
	NOTIFYICONDATA nid;
	ZeroMemory(&nid, sizeof(nid));
	nid.cbSize = sizeof(nid);
	nid.hWnd = GetParent(g_hwndDllPath);
	nid.uID = 100;
	nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_INFO;
	nid.uCallbackMessage = WM_USER + 1;
	nid.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_MAIN_ICON));

	wcsncpy_s(nid.szTip, L"Fediska Injector", _TRUNCATE);

	nid.dwInfoFlags = success ? NIIF_INFO : NIIF_ERROR;
	wcsncpy_s(nid.szInfoTitle, title, _TRUNCATE);
	wcsncpy_s(nid.szInfo, message, _TRUNCATE);
	nid.uTimeout = 5000;

	Shell_NotifyIcon(NIM_ADD, &nid);
	Shell_NotifyIcon(NIM_SETVERSION, &nid);
	Shell_NotifyIcon(NIM_MODIFY, &nid);
	Shell_NotifyIcon(NIM_DELETE, &nid);
}

void UpdatePidStatus(HWND hwndMain) {
	if (!g_hwndProcName || !g_hwndPidStatus) return;

	wchar_t procName[MAX_PATH];
	GetWindowText(g_hwndProcName, procName, MAX_PATH);

	DWORD pid = 0;

	//Поиск по PID
	DWORD manualPid = _wtoi(procName);

	if (manualPid > 0) {
		// ИСПОЛЬЗУЕМ SYNCHRONIZE (0x00100000) - минимальное право для проверки, что процесс существует
		HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, manualPid);
		if (hProcess != NULL) {
			CloseHandle(hProcess);
			pid = manualPid;
		}
	}

	if (pid == 0) {
		// Если не PID или PID не найден, ищем по имени
		pid = GetProcessIdByName(procName);
	}

	wchar_t statusText[50];
	if (pid > 0) {
		// Обновляем текст статуса, чтобы он показывал найденный PID
		swprintf(statusText, 50, L"✅ FOUND (PID: %lu)", pid);
		SetWindowText(g_hwndPidStatus, statusText);
		InvalidateRect(g_hwndPidStatus, NULL, TRUE);
	}
	else {
		SetWindowText(g_hwndPidStatus, L"❌ NOT FOUND");
		InvalidateRect(g_hwndPidStatus, NULL, TRUE);
	}
}


bool CheckFileExists(const wchar_t* path)
{
	return (GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES);
}

BOOL SetDebugPrivilege(BOOL bEnable)
{
	HANDLE hToken;
	LUID luid;
	TOKEN_PRIVILEGES tp;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) { return FALSE; }
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) { CloseHandle(hToken); return FALSE; }

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if (bEnable) tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else tp.Privileges[0].Attributes = 0;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) { CloseHandle(hToken); return FALSE; }
	CloseHandle(hToken);
	return TRUE;
}

DWORD GetProcessIdByName(const wchar_t* processName) {
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE) { return 0; }

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hSnap, &pe32)) { CloseHandle(hSnap); return 0; }

	DWORD pid = 0;
	do {
		if (_wcsicmp(pe32.szExeFile, processName) == 0) {
			pid = pe32.th32ProcessID;
			break;
		}
	} while (Process32Next(hSnap, &pe32));

	CloseHandle(hSnap);
	return pid;
}

BOOL InjectDll(DWORD pid, const wchar_t* dllPath) {
	if (pid == 0 || !CheckFileExists(dllPath)) return FALSE;

	HANDLE hProcess = NULL;
	HMODULE hNtdll = GetModuleHandle(L"ntdll.dll");
	PNTOPENPROCESS NtOpenProcess = (PNTOPENPROCESS)GetProcAddress(hNtdll, "NtOpenProcess");

	if (NtOpenProcess == NULL) { return FALSE; }

	OBJECT_ATTRIBUTES objAttr;
	CLIENT_ID clientId;
	NTSTATUS status;
	ACCESS_MASK desiredAccess = PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ;

	InitializeObjectAttributes(&objAttr, NULL, 0, NULL, NULL);
	clientId.UniqueProcess = (HANDLE)pid;
	clientId.UniqueThread = NULL;

	// NtOpenProcess часто обходит стандартные проверки безопасности Windows (такие как OpenProcess), 
	// но права (desiredAccess) все равно должны быть достаточными.
	status = NtOpenProcess(&hProcess, desiredAccess, &objAttr, &clientId);

	if (status != 0 || hProcess == NULL) { return FALSE; }

	size_t dllPathSize = (wcslen(dllPath) + 1) * sizeof(wchar_t);
	LPVOID allocatedMem = VirtualAllocEx(hProcess, NULL, dllPathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (allocatedMem == NULL) { CloseHandle(hProcess); return FALSE; }

	if (WriteProcessMemory(hProcess, allocatedMem, dllPath, dllPathSize, NULL) == 0) {
		VirtualFreeEx(hProcess, allocatedMem, 0, MEM_RELEASE); CloseHandle(hProcess); return FALSE;
	}

	HMODULE hKernel32 = GetModuleHandle(L"kernel32.dll");
	LPTHREAD_START_ROUTINE loadLibraryAddr = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryW");

	if (loadLibraryAddr == NULL) {
		VirtualFreeEx(hProcess, allocatedMem, 0, MEM_RELEASE); CloseHandle(hProcess);
		return FALSE;
	}
	else {
		HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0, loadLibraryAddr, allocatedMem, 0, NULL);
		if (hRemoteThread == NULL) {
			VirtualFreeEx(hProcess, allocatedMem, 0, MEM_RELEASE); CloseHandle(hProcess);
			return FALSE;
		}
		else {
			WaitForSingleObject(hRemoteThread, INFINITE);
			CloseHandle(hRemoteThread);
		}
	}

	if (allocatedMem) { VirtualFreeEx(hProcess, allocatedMem, 0, MEM_RELEASE); }
	if (hProcess) { CloseHandle(hProcess); }
	return TRUE;
}


void ChooseTextColor(HWND hwnd) {
	CHOOSECOLORW cc;
	ZeroMemory(&cc, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = hwnd;
	cc.rgbResult = g_crTextColor;
	cc.lpCustColors = g_ccCustomColors;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (ChooseColorW(&cc) == TRUE) {
		g_crTextColor = cc.rgbResult;
		if (g_hDarkBrush) DeleteObject(g_hDarkBrush);
		g_hDarkBrush = CreateSolidBrush(RGB(30, 30, 30));
		InvalidateRect(hwnd, NULL, TRUE);
	}
}

void ChooseGradTopColor(HWND hwnd) {
	CHOOSECOLORW cc;
	ZeroMemory(&cc, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = hwnd;
	cc.rgbResult = g_crGradTop;
	cc.lpCustColors = g_ccCustomColors;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (ChooseColorW(&cc) == TRUE) {
		g_crGradTop = cc.rgbResult;
		InvalidateRect(hwnd, NULL, TRUE);
	}
}

void ChooseGradBottomColor(HWND hwnd) {
	CHOOSECOLORW cc;
	ZeroMemory(&cc, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = hwnd;
	cc.rgbResult = g_crGradBottom;
	cc.lpCustColors = g_ccCustomColors;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (ChooseColorW(&cc) == TRUE) {
		g_crGradBottom = cc.rgbResult;
		InvalidateRect(hwnd, NULL, TRUE);
	}
}

void CheckDllStatus() {
	wchar_t procNameBuffer[MAX_PATH];
	wchar_t dllPathBuffer[MAX_PATH];

	GetWindowText(g_hwndProcName, procNameBuffer, MAX_PATH);
	GetWindowText(g_hwndDllPath, dllPathBuffer, MAX_PATH);

	DWORD pid = 0;
	DWORD manualPid = _wtoi(procNameBuffer);

	if (manualPid > 0) {
		// ИСПОЛЬЗУЕМ SYNCHRONIZE (минимальное право)
		HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, manualPid);
		if (hProcess != NULL) {
			CloseHandle(hProcess);
			pid = manualPid;
		}
	}

	if (pid == 0) {
		pid = GetProcessIdByName(procNameBuffer);
	}

	if (pid == 0) {
		std::wstring errMsg = L"Процесс не найден!";
		ShowMessage(L"❌ Ошибка", errMsg.c_str(), false);
		return;
	}

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		ShowMessage(L"❌ Ошибка", (L"Не удалось получить список модулей процесса (PID: " + std::to_wstring(pid) + L"). Код: " + std::to_wstring(GetLastError())).c_str(), false);
		return;
	}

	MODULEENTRY32 me32;
	me32.dwSize = sizeof(MODULEENTRY32);

	std::wstring dllFileName = dllPathBuffer;
	size_t last_slash = dllFileName.find_last_of(L"\\/");
	if (last_slash != std::wstring::npos) {
		dllFileName = dllFileName.substr(last_slash + 1);
	}

	bool found = false;
	if (Module32First(hSnapshot, &me32)) {
		do {
			if (_wcsicmp(me32.szModule, dllFileName.c_str()) == 0) {
				found = true;
				break;
			}
		} while (Module32Next(hSnapshot, &me32));
	}

	CloseHandle(hSnapshot);

	if (found) {
		std::wstring successMsg = L"Модуль '" + dllFileName + L"' НАЙДЕН в процессе (PID: " + std::to_wstring(pid) + L")";
		ShowMessage(L"✅ Статус: Успех!", successMsg.c_str(), true);
	}
	else {
		std::wstring failMsg = L"Модуль '" + dllFileName + L"' НЕ НАЙДЕН в процессе (PID: " + std::to_wstring(pid) + L")";
		ShowMessage(L"❌ Статус: Ошибка!", failMsg.c_str(), false);
	}
}

//Убираем строгую проверку PID и сразу доверяем введенным цифрам
void PerformInjection() {
	wchar_t procNameBuffer[MAX_PATH];
	wchar_t dllPathBuffer[MAX_PATH];

	GetWindowText(g_hwndProcName, procNameBuffer, MAX_PATH);
	GetWindowText(g_hwndDllPath, dllPathBuffer, MAX_PATH);

	DWORD pid = 0;
	std::wstring procNameStr(procNameBuffer);

	// 1. Пытаемся найти по PID
	DWORD manualPid = _wtoi(procNameBuffer);

	if (manualPid > 0) {
		pid = manualPid;
		procNameStr = L"PID: " + std::to_wstring(pid);
	}

	if (pid == 0) {
		// 2. Если не PID (или PID был 0), ищем по имени
		pid = GetProcessIdByName(procNameBuffer);
	}

	if (pid == 0) {
		ShowMessage(L"⏳ Ожидание", (L"Процесс '" + procNameStr + L"' не найден. Ожидаю запуска (до 15 секунд)...").c_str(), true);

		for (int i = 0; i < 30; ++i) { // 30 итераций * 500мс = 15 секунд ожидания
			Sleep(500);

			// Повторяем логику поиска PID/Имени
			DWORD currentPid = 0;
			DWORD checkPid = _wtoi(procNameBuffer);
			if (checkPid > 0) {
				// В цикле ожидания тоже доверяем PID
				currentPid = checkPid;
			}

			if (currentPid == 0) {
				currentPid = GetProcessIdByName(procNameBuffer);
			}

			if (currentPid > 0) {
				pid = currentPid;
				procNameStr = L"Found (PID: " + std::to_wstring(pid) + L")";
				break; // Нашли! Выходим из цикла ожидания.
			}
		}
	}


	//ФИНАЛЬНАЯ ПРОВЕРКА
	if (pid == 0) {
		std::wstring errMsg = L"Процесс '" + procNameStr + L"' так и не был запущен за 15 секунд. Попробуй еще раз.";
		ShowMessage(L"❌ Ошибка инжекта", errMsg.c_str(), false);
		return;
	}

	if (!CheckFileExists(dllPathBuffer)) {
		ShowMessage(L"❌ Ошибка инжекта", L"DLL-файл не найден по указанному пути!", false);
		return;
	}

	if (InjectDll(pid, dllPathBuffer)) {
		ShowMessage(L"🎉 УСПЕХ!", L"DLL ИНЖЕКТИРОВАНА! (Стелс-закрытие через 2с)", true);
		CheckDllStatus();
		PlaySoundW(L"SystemExclamation", NULL, SND_ALIAS | SND_ASYNC);

		//ЗАКРЫТИЕ
		Sleep(2000);
		DestroyWindow(GetParent(g_hwndDllPath));

	}
	else {
		// Если инжект не удался (код ошибки), это, скорее всего, из-за защиты (Access Denied)
		std::wstring errMsg = L"Инжект не удался в процесс (PID: " + std::to_wstring(pid) + L")! Скорее всего, это защита. Код ошибки: " + std::to_wstring(GetLastError());
		ShowMessage(L"❌ ФИНАЛЬНАЯ ОШИБКА", errMsg.c_str(), false);
	}
}

void LoadSettings() {
	wchar_t procName[MAX_PATH];
	wchar_t dllPath[MAX_PATH];

	// Загружаем пути
	GetPrivateProfileStringW(L"Settings", L"ProcessName", L"TargetProcess.exe", procName, MAX_PATH, L"config.ini");
	GetPrivateProfileStringW(L"Settings", L"DllPath", L"C:\\Users\\Public\\My_CSS_Hack.dll", dllPath, MAX_PATH, L"config.ini");
	wcscpy_s(g_procNameBuffer, MAX_PATH, procName);
	wcscpy_s(g_dllPath, MAX_PATH, dllPath);

	// Загружаем цвета
	g_crTextColor = GetPrivateProfileIntW(L"Style", L"TextColor", RGB(255, 255, 255), L"config.ini");
	g_crGradTop = GetPrivateProfileIntW(L"Style", L"GradTop", RGB(64, 0, 0), L"config.ini");
	g_crGradBottom = GetPrivateProfileIntW(L"Style", L"GradBottom", RGB(30, 30, 30), L"config.ini");

	if (GetRValue(g_crGradTop) > 100 || GetGValue(g_crGradTop) > 100 || GetBValue(g_crGradTop) > 100) {
		g_crGradTop = RGB(64, 0, 0);
	}
	if (GetRValue(g_crGradBottom) > 100 || GetGValue(g_crGradBottom) > 100 || GetBValue(g_crGradBottom) > 100) {
		g_crGradBottom = RGB(30, 30, 30);
	}
	

	// Загружаем заметки
	GetPrivateProfileStringW(
		L"Style",
		L"Notes",
		L"Insert your notes here...",
		g_notesBuffer,
		MAX_NOTES_LENGTH,
		L"config.ini"
	);

	GetPrivateProfileStringW(
		L"Settings",
		L"HotkeyVK",
		L"120", // Значение по умолчанию: VK_F9 (120)
		g_hotKeyVkStr,
		10,
		L"config.ini"
	);

	// Преобразуем загруженный текст в число (UINT)
	g_hotKeyVk = _wtoi(g_hotKeyVkStr);
	g_hotKeyMod = MOD_CONTROL;
}

void SaveSettings() {
	wchar_t procName[MAX_PATH];
	wchar_t dllPath[MAX_PATH];
	wchar_t colorBuffer[16];

	//СОХРАНЕНИЕ ПУТЕЙ И ЦВЕТОВ
	if (g_hwndProcName) GetWindowText(g_hwndProcName, procName, MAX_PATH); else wcscpy_s(procName, MAX_PATH, L"TargetProcess.exe");
	if (g_hwndDllPath) GetWindowText(g_hwndDllPath, dllPath, MAX_PATH); else wcscpy_s(dllPath, MAX_PATH, g_dllPath);

	WritePrivateProfileStringW(L"Settings", L"ProcessName", procName, L"config.ini");
	WritePrivateProfileStringW(L"Settings", L"DllPath", dllPath, L"config.ini");

	// Сохранение цветов
	_itow_s(g_crTextColor, colorBuffer, 16, 10); WritePrivateProfileStringW(L"Style", L"TextColor", colorBuffer, L"config.ini");
	_itow_s(g_crGradTop, colorBuffer, 16, 10); WritePrivateProfileStringW(L"Style", L"GradTop", colorBuffer, L"config.ini");
	_itow_s(g_crGradBottom, colorBuffer, 16, 10); WritePrivateProfileStringW(L"Style", L"GradBottom", colorBuffer, L"config.ini");


	//  СОХРАНЕНИЕ ЗАМЕТОК 
	WritePrivateProfileStringW(L"Style", L"Notes", g_notesBuffer, L"config.ini");

	// Сохранение Hotkey VK Code
	wchar_t vkCodeBuffer[10];
	_itow_s(g_hotKeyVk, vkCodeBuffer, 10, 10); // Преобразуем число в текст
	WritePrivateProfileStringW(L"Settings", L"HotkeyVK", vkCodeBuffer, L"config.ini");
}



//Обработчик окна настроек (SettingsWndProc)

LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	// Получаем текущие настройки для отображения
	wchar_t hotkeyText[MAX_PATH];
	// Определяем номер F-клавиши (F1 = 112, F9 = 120, F10 = 121)
	int f_num = g_hotKeyVk - VK_F1 + 1;
	wsprintf(hotkeyText, L"Current: Ctrl + F%d", f_num);

	switch (msg) {
	case WM_CREATE:
	{
		//СЕКЦИЯ ЦВЕТОВ (GROUPBOX)
		CreateWindowEx(0, L"BUTTON", L"Colors & Style", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 5, 440, 80, hwnd, NULL, g_hInst, NULL);

		// Кнопки для выбора цвета (Добавлен BS_OWNERDRAW)
		CreateWindowEx(0, L"BUTTON", L"1. Text Color", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 15, 25, 100, 25, hwnd, (HMENU)IDC_BTN_COLOR, g_hInst, NULL);
		CreateWindowEx(0, L"BUTTON", L"2. Grad Top", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 125, 25, 100, 25, hwnd, (HMENU)IDC_BTN_GRAD_TOP, g_hInst, NULL);
		CreateWindowEx(0, L"BUTTON", L"3. Grad Bottom", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 235, 25, 100, 25, hwnd, (HMENU)IDC_BTN_GRAD_BOTTOM, g_hInst, NULL);

		// Кнопка сохранения (Добавлен BS_OWNERDRAW)
		CreateWindowEx(0, L"BUTTON", L"Save All Settings", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 345, 25, 100, 25, hwnd, (HMENU)IDC_BTN_SAVE_COLORS, g_hInst, NULL);

		// СЕКЦИЯ ТЕМ 
		CreateWindowEx(0, L"BUTTON", L"Themes Pack", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 95, 210, 110, hwnd, NULL, g_hInst, NULL);

		g_hwndThemesList = CreateWindowEx(
			WS_EX_CLIENTEDGE, L"LISTBOX", NULL,
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS,
			15, 115, 190, 80,
			hwnd, (HMENU)IDC_LIST_THEMES, g_hInst, NULL);

		// Заполняем список тем
		for (size_t i = 0; i < g_themeCount; ++i) {
			SendMessage(g_hwndThemesList, LB_ADDSTRING, 0, (LPARAM)g_themes[i].name);
		}

		// 3. СЕКЦИЯ ЗАМЕТОК 
		CreateWindowEx(0, L"BUTTON", L"Notes", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 225, 95, 220, 110, hwnd, NULL, g_hInst, NULL);
		g_hwndNotes = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", g_notesBuffer, WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL, 235, 115, 200, 80, hwnd, (HMENU)IDC_EDIT_NOTES, g_hInst, NULL);

		// СЕКЦИЯ ХОТКЕЯ
		CreateWindowEx(0, L"BUTTON", L"Hotkey", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 215, 440, 50, hwnd, NULL, g_hInst, NULL);
		CreateWindowEx(0, L"BUTTON", L"Set Hotkey (Ctrl+F10)", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 15, 235, 140, 25, hwnd, (HMENU)IDC_BTN_HOTKEY, g_hInst, NULL);
		CreateWindowEx(0, L"STATIC", hotkeyText, WS_CHILD | WS_VISIBLE | SS_LEFT, 165, 240, 150, 20, hwnd, (HMENU)IDC_STATIC_HOTKEY_DISPLAY, g_hInst, NULL);


		// КНОПКА ЗАКРЫТИЯ
		CreateWindowEx(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_OWNERDRAW, 360, 270, 85, 25, hwnd, (HMENU)IDOK, g_hInst, NULL);

		// Применяем шрифт ко всем элементам
		HWND hChild = GetTopWindow(hwnd);
		while (hChild) {
			SendMessage(hChild, WM_SETFONT, (WPARAM)g_hFont, TRUE);
			hChild = GetNextWindow(hChild, GW_HWNDNEXT);
		}
	}
	break;

	case WM_DRAWITEM:
	{
		LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;

		// Проверяем, что это вообще кнопка, которую мы должны рисовать
		if (pdis->CtlType != ODT_BUTTON) return DefWindowProc(hwnd, msg, wParam, lParam);

		COLORREF bgColor = RGB(50, 50, 50); // Дефолтный фон для всех
		COLORREF fgColor = g_crTextColor; // Текст

		// Специальное правило: для "Save All Settings" делаем красный фон
		if (pdis->CtlID == IDC_BTN_SAVE_COLORS) {
			bgColor = RGB(64, 0, 0);
		}

		// Если кнопка нажата, делаем ее темнее
		if (pdis->itemState & ODS_SELECTED) {
			bgColor = RGB(150, 0, 0); // ТЕМНО-КРАСНЫЙ при нажатии
		}
		// Если курсор наведен, делаем ее яркой
		else if (pdis->itemState & ODS_COMBOBOXEDIT) {
			bgColor = (pdis->CtlID == IDC_BTN_SAVE_COLORS) ? RGB(255, 0, 0) : RGB(80, 80, 80); // ЯРКО-КРАСНЫЙ только для Save, серый для остальных
		}

		// 1. Рисуем фон (квадрат)
		HBRUSH hBrush = CreateSolidBrush(bgColor);
		FillRect(pdis->hDC, &pdis->rcItem, hBrush);
		DeleteObject(hBrush);

		// 2. Рисуем рамку (по желанию)
		DrawEdge(pdis->hDC, &pdis->rcItem, EDGE_RAISED, BF_RECT);

		// 3. Рисуем текст
		SetBkMode(pdis->hDC, TRANSPARENT);
		SetTextColor(pdis->hDC, fgColor);

		wchar_t text[64];
		GetWindowTextW(pdis->hwndItem, text, 64);

		// Выравниваем текст по центру
		DrawTextW(pdis->hDC, text, -1, &pdis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		return TRUE;
	}
	break;

	case WM_CTLCOLORDLG:
	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLOREDIT:
	{
		HDC hdc = (HDC)wParam;
		SetTextColor(hdc, g_crTextColor);
		SetBkColor(hdc, RGB(30, 30, 30));
		return (LRESULT)g_hDarkBrush;
	}

	case WM_CTLCOLORBTN:
	{
		HDC hdc = (HDC)wParam;
		HWND hButton = (HWND)lParam;

		// Этот case нужен только для GroupBox'ов и стандартных кнопок. 
		// GroupBox
		if (GetDlgCtrlID(hButton) < 2000) {
			SetTextColor(hdc, g_crTextColor);
			SetBkColor(hdc, RGB(30, 30, 30));
			return (LRESULT)g_hDarkBrush;
		}

		// Стандартные кнопки (если остались)
		SetTextColor(hdc, g_crTextColor);
		SetBkColor(hdc, RGB(50, 50, 50));
		return (LRESULT)CreateSolidBrush(RGB(50, 50, 50));
	}

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			DestroyWindow(hwnd); // Закрываем окно
			return 0;

		case IDC_BTN_COLOR:
			ChooseTextColor(GetParent(hwnd));
			break;
		case IDC_BTN_GRAD_TOP:
			ChooseGradTopColor(GetParent(hwnd));
			break;
		case IDC_BTN_GRAD_BOTTOM:
			ChooseGradBottomColor(GetParent(hwnd));
			break;
		case IDC_BTN_SAVE_COLORS:
			// 1. Считываем заметки
			GetWindowText(g_hwndNotes, g_notesBuffer, MAX_NOTES_LENGTH);
			SaveSettings();
			ShowMessage(L"✅ Сохранено", L"Настройки цвета, заметок и хоткея сохранены в config.ini!", true);
			break;
		case IDC_BTN_HOTKEY:
		{ 
			g_hotKeyMod = MOD_CONTROL;
			g_hotKeyVk = VK_F10;

			// Нужно обновить хоткей в системе
			UnregisterHotKey(GetParent(hwnd), 1);
			RegisterHotKey(GetParent(hwnd), 1, g_hotKeyMod, g_hotKeyVk);

			// Обновляем текст в окне
			int f_num_new = g_hotKeyVk - VK_F1 + 1;
			wsprintf(hotkeyText, L"Current: Ctrl + F%d", f_num_new);
			SetDlgItemText(hwnd, IDC_STATIC_HOTKEY_DISPLAY, hotkeyText);

			ShowMessage(L"✅ Хоткей", L"Горячая клавиша изменена на Ctrl + F10!", true);
		} //ЗАКРЫВАЮЩАЯ СКОБКА ЛОКАЛЬНОГО БЛОКА
		break;

		case IDC_LIST_THEMES:
			if (HIWORD(wParam) == LBN_SELCHANGE) {
				// Пользователь выбрал тему
				LRESULT index = SendMessage(g_hwndThemesList, LB_GETCURSEL, 0, 0);

				if (index != LB_ERR) {
					// 1. Применяем цвета из выбранной темы
					g_crTextColor = g_themes[index].textColor;
					g_crGradTop = g_themes[index].gradTop;
					g_crGradBottom = g_themes[index].gradBottom;

					// 2. Обновляем кисть для фона 
					if (g_hDarkBrush) DeleteObject(g_hDarkBrush);
					g_hDarkBrush = CreateSolidBrush(RGB(30, 30, 30));

					// 3. Сохраняем и уведомляем
					SaveSettings();
					ShowMessage(L"✅ Тема", (L"Применена тема: " + std::wstring(g_themes[index].name)).c_str(), true);

					// 4. Перерисовываем главное окно
					InvalidateRect(GetParent(hwnd), NULL, TRUE);
				}
			}
			break;
		}
		break;

	case WM_DESTROY:
		// При закрытии окна настроек нужно обновить главное окно
		InvalidateRect(GetParent(hwnd), NULL, TRUE);
		UnregisterClass(L"SettingsWindowClass", g_hInst);
		break;

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

// 🔥 Запуск окна настроек

void OpenSettingsWindow(HWND hParent) {
	// РЕГИСТРАЦИЯ КЛАССА ОКНА НАСТРОЕК
	WNDCLASSEX wc = { 0 };
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = SettingsWndProc;
	wc.hInstance = g_hInst;
	wc.lpszClassName = L"SettingsWindowClass";
	wc.hbrBackground = CreateSolidBrush(RGB(30, 30, 30));

	if (!RegisterClassEx(&wc)) {
		// Класс уже зарегистрирован, это нормально
	}

	// СОЗДАНИЕ ОКНА НАСТРОЕК
	HWND hwndSettings = CreateWindowEx(
		0,
		L"SettingsWindowClass",
		L"Injector Style & Settings",
		WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
		CW_USEDEFAULT, CW_USEDEFAULT, 460, 330,
		hParent,
		NULL,
		g_hInst,
		NULL);

	if (hwndSettings) {
		ShowWindow(hwndSettings, SW_SHOW);
		UpdateWindow(hwndSettings);
	}
}


// Функция обработки сообщений главного окна (WndProc)
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
	{
		if (g_hDarkBrush) DeleteObject(g_hDarkBrush);
		g_hDarkBrush = CreateSolidBrush(RGB(30, 30, 30));

		// 1. Process Name
		CreateWindowEx(0, L"STATIC", L"Process Name:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 15, 90, 20, hwnd, (HMENU)1, g_hInst, NULL);
		g_hwndProcName = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", g_procNameBuffer, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 100, 10, 290, 25, hwnd, (HMENU)IDC_PROC_NAME, g_hInst, NULL);
		CreateWindowEx(0, L"BUTTON", L"Select Proc", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 400, 10, 80, 25, hwnd, (HMENU)IDC_BTN_SELECT, g_hInst, NULL);

		g_hwndPidStatus = CreateWindowEx(0, L"STATIC", L"---", WS_CHILD | WS_VISIBLE | SS_CENTER, 490, 13, 70, 20, hwnd, (HMENU)IDC_STATIC_PID_STATUS, g_hInst, NULL);


		// ГРУППА "DLL Info"
		CreateWindowEx(0, L"BUTTON", L"DLL Info", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 45, 555, 100, hwnd, (HMENU)3, g_hInst, NULL);
		CreateWindowEx(0, L"STATIC", L"DLL Path:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 65, 90, 20, hwnd, (HMENU)4, g_hInst, NULL);
		g_hwndDllPath = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", g_dllPath, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 100, 60, 370, 25, hwnd, (HMENU)IDC_DLL_PATH, g_hInst, NULL);
		CreateWindowEx(0, L"BUTTON", L"Select DLL", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 480, 60, 80, 25, hwnd, (HMENU)IDC_BTN_SELECT_DLL, g_hInst, NULL);
		CreateWindowEx(0, L"BUTTON", L"Open Folder", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 480, 95, 80, 25, hwnd, (HMENU)IDC_BTN_OPEN_FOLDER, g_hInst, NULL);


		// ГЛАВНЫЕ КНОПКИ ДЕЙСТВИЙ (y=175)
		CreateWindowEx(0, L"BUTTON", L"About", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 10, 175, 70, 25, hwnd, (HMENU)IDC_BTN_ABOUT, g_hInst, NULL);
		CreateWindowEx(0, L"BUTTON", L"Themes", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 85, 175, 70, 25, hwnd, (HMENU)IDC_BTN_SETTINGS, g_hInst, NULL);
		CreateWindowEx(0, L"BUTTON", L"Check Status", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 355, 175, 110, 25, hwnd, (HMENU)IDC_BTN_CHECK_STATUS, g_hInst, NULL);
		CreateWindowEx(0, L"BUTTON", L"Inject", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 470, 175, 80, 25, hwnd, (HMENU)IDC_BTN_INJECT, g_hInst, NULL);


		// Применяем шрифты
		HWND hChild = GetTopWindow(hwnd);
		while (hChild) {
			SendMessage(hChild, WM_SETFONT, (WPARAM)g_hFont, TRUE);
			hChild = GetNextWindow(hChild, GW_HWNDNEXT);
		}

		// ЗАПУСКАЕМ ТАЙМЕР ДЛЯ АВТОМАТИЧЕСКОЙ ПРОВЕРКИ СТАТУСА
		SetTimer(hwnd, TIMER_ID_STATUS, 1500, NULL);
		UpdatePidStatus(hwnd);

		// ЗАПУСКАЕМ ТАЙМЕР ДЛЯ ПЕРЕЛИВАЮЩЕГОСЯ ГРАДИЕНТА (50 мс)
		SetTimer(hwnd, TIMER_ID_ANIMATION, 50, NULL);
	}
	break;

	case WM_TIMER:
		if (wParam == TIMER_ID_HOVER)
		{
			POINT pt;
			GetCursorPos(&pt);
			HWND hWindow = WindowFromPoint(pt);

			if (hWindow != g_hwndHover)
			{
				KillTimer(hwnd, TIMER_ID_HOVER);
				InvalidateRect(g_hwndHover, NULL, TRUE);
				g_hwndHover = NULL;
			}
		}
		else if (wParam == TIMER_ID_STATUS)
		{
			UpdatePidStatus(hwnd);
		}
		//  ЛОГИКА АНИМАЦИИ ГРАДИЕНТА 
		else if (wParam == TIMER_ID_ANIMATION)
		{
			COLORREF currentTop = g_crGradTop;
			COLORREF currentBottom = g_crGradBottom;

			// ЛОГИКА ДЛЯ ВЕРХНЕГО ЦВЕТА (g_crGradTop)

			// R-канал
			int R = GetRValue(currentTop);
			R += g_dirR * 1;
			if (R > 80) g_dirR = -1;
			if (R < 30) g_dirR = 1;
			R = min(max(R, 30), 80);

			// G-канал
			int G = GetGValue(currentTop);
			G += g_dirG * 1;
			if (G > 40) g_dirG = -1;
			if (G < 0) g_dirG = 1;
			G = min(max(G, 0), 40);

			// B-канал
			int B = GetBValue(currentTop);
			B += g_dirB * 1;
			if (B > 40) g_dirB = -1;
			if (B < 0) g_dirB = 1;
			B = min(max(B, 0), 40);

			g_crGradTop = RGB(R, G, B);

			//ЛОГИКА ДЛЯ НИЖНЕГО ЦВЕТА (g_crGradBottom)

			// R-канал
			int R2 = GetRValue(currentBottom);
			R2 += g_dirR2 * 1;
			if (R2 > 50) g_dirR2 = -1;
			if (R2 < 10) g_dirR2 = 1;
			R2 = min(max(R2, 10), 50);

			// G-канал
			int G2 = GetGValue(currentBottom);
			G2 += g_dirG2 * 1;
			if (G2 > 50) g_dirG2 = -1;
			if (G2 < 10) g_dirG2 = 1;
			G2 = min(max(G2, 10), 50);

			// B-канал
			int B2 = GetBValue(currentBottom);
			B2 += g_dirB2 * 1;
			if (B2 > 50) g_dirB2 = -1;
			if (B2 < 10) g_dirB2 = 1;
			B2 = min(max(B2, 10), 50);

			g_crGradBottom = RGB(R2, G2, B2);

			// ПЕРЕРИСОВКА ОКНА С НОВЫМИ ЦВЕТАМИ
			InvalidateRect(hwnd, NULL, FALSE);
		}
		return 0;

	case WM_HOTKEY:
		if (wParam == 1)
		{
			ShowMessage(L"🔥 Хоткей", L"Горячая клавиша сработала! Начинаю инжект...", true);
			PerformInjection();
		}
		break;

	case WM_MOUSEMOVE:
	{
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(hwnd, &pt);
		HWND hChild = ChildWindowFromPoint(hwnd, pt);

		if (hChild && (GetWindowLong(hChild, GWL_STYLE) & BS_PUSHBUTTON))
		{
			if (g_hwndHover != hChild)
			{
				if (g_hwndHover) { InvalidateRect(g_hwndHover, NULL, TRUE); }
				g_hwndHover = hChild;
				InvalidateRect(g_hwndHover, NULL, TRUE);
				SetTimer(hwnd, TIMER_ID_HOVER, 10, NULL);
			}
		}
	}
	break;

	case WM_ERASEBKGND:
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		RECT rc;
		HDC hdc;

		if (msg == WM_PAINT) { hdc = BeginPaint(hwnd, &ps); }
		else { hdc = (HDC)wParam; }

		GetClientRect(hwnd, &rc);

		// Градиент
		TRIVERTEX tv[2] = {
			{ rc.left, rc.top, (USHORT)(GetRValue(g_crGradTop) << 8 | GetRValue(g_crGradTop)), (USHORT)(GetGValue(g_crGradTop) << 8 | GetGValue(g_crGradTop)), (USHORT)(GetBValue(g_crGradTop) << 8 | GetBValue(g_crGradTop)), 0x0000 },
			{ rc.right, rc.bottom, (USHORT)(GetRValue(g_crGradBottom) << 8 | GetRValue(g_crGradBottom)), (USHORT)(GetGValue(g_crGradBottom) << 8 | GetGValue(g_crGradBottom)), (USHORT)(GetBValue(g_crGradBottom) << 8 | GetBValue(g_crGradBottom)), 0x0000 }
		};

		GRADIENT_RECT gr = { 0, 1 };

		GradientFill(hdc, tv, 2, &gr, 1, GRADIENT_FILL_RECT_V);

		if (msg == WM_PAINT) { EndPaint(hwnd, &ps); }
	}
	return 0;

	case WM_CTLCOLORBTN:
	{
		HDC hdc = (HDC)wParam;
		HWND hButton = (HWND)lParam;

		if (hButton == g_hwndHover)
		{
			SetTextColor(hdc, g_crTextColor);
			SetBkColor(hdc, RGB(255, 0, 0));
			return (LRESULT)CreateSolidBrush(RGB(255, 0, 0));
		}
		SetTextColor(hdc, g_crTextColor);
		SetBkColor(hdc, RGB(50, 50, 50));
		return (LRESULT)CreateSolidBrush(RGB(50, 50, 50));
	}

	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC:
	{
		HDC hdc = (HDC)wParam;
		HWND hStatic = (HWND)lParam;

		// ЦВЕТ ДЛЯ ИНДИКАТОРА СТАТУСА
		if (hStatic == g_hwndPidStatus) {
			wchar_t text[50];
			GetWindowTextW(hStatic, text, 50);

			if (wcsstr(text, L"✅")) {
				SetTextColor(hdc, RGB(0, 255, 0)); // Зеленый текст
			}
			else if (wcsstr(text, L"❌")) {
				SetTextColor(hdc, RGB(255, 0, 0)); // Красный текст
			}
			else {
				SetTextColor(hdc, RGB(255, 255, 0)); // Желтый текст
			}
		}
		else {
			SetTextColor(hdc, g_crTextColor);
		}

		SetBkColor(hdc, RGB(30, 30, 30));
		return (LRESULT)g_hDarkBrush;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BTN_INJECT:
			PerformInjection();
			break;

		case IDC_BTN_CHECK_STATUS:
			CheckDllStatus();
			UpdatePidStatus(hwnd);
			break;

		case IDC_BTN_OPEN_FOLDER:
		{
			wchar_t dllPath[MAX_PATH];
			GetWindowText(g_hwndDllPath, dllPath, MAX_PATH);

			std::wstring path = dllPath;
			size_t last_slash = path.find_last_of(L"\\/");

			if (last_slash != std::wstring::npos) {
				std::wstring folderPath = path.substr(0, last_slash);
				ShellExecute(NULL, L"explore", folderPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
			}
			else {
				ShowMessage(L"❌ Ошибка", L"Невозможно определить папку DLL. Проверь путь.", false);
			}
		}
		break;

		case IDC_BTN_SETTINGS: // КНОПКА THEMES
			// ЗАПУСКАЕМ НОВОЕ ОКНО НАСТРОЕК (НЕ ДИАЛОГ)
			OpenSettingsWindow(hwnd);

			// Маскировка
			SetWindowText(hwnd, g_szMaskedTitle);
			SetClassLongPtr(hwnd, GCLP_HICON, (LONG_PTR)g_hGameIcon);
			SetClassLongPtr(hwnd, GCLP_HICONSM, (LONG_PTR)g_hGameIcon);

			ShowMessage(L"✅ Маскировка", (L"Активирован режим маскировки!\n\nЗаголовок: '" + std::wstring(g_szMaskedTitle) + L"'").c_str(), true);
			break;

		case IDC_BTN_SELECT: // Select Proc
		{
			OPENFILENAME ofn;
			wchar_t szFile[MAX_PATH] = { 0 };
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hwnd;
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

			ofn.lpstrFilter = L"Executable Files (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";

			if (GetOpenFileName(&ofn) == TRUE)
			{
				std::wstring fullPath(szFile);
				size_t last_slash = fullPath.find_last_of(L"\\/");
				std::wstring fileName = (last_slash == std::wstring::npos) ? fullPath : fullPath.substr(last_slash + 1);
				SetWindowText(g_hwndProcName, fileName.c_str());
				UpdatePidStatus(hwnd); // Обновляем статус после выбора процесса
			}
		}
		break;

		case IDC_BTN_SELECT_DLL: // Select DLL
		{
			OPENFILENAME ofn;
			wchar_t szFile[MAX_PATH] = { 0 };
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hwnd;
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

			ofn.lpstrFilter = L"DLL Files (*.dll)\0*.dll\0All Files (*.*)\0*.*\0";

			if (GetOpenFileName(&ofn) == TRUE)
			{
				wcscpy_s(g_dllPath, MAX_PATH, szFile);
				SetWindowText(g_hwndDllPath, szFile);
			}
		}
		break;

		case IDC_BTN_ABOUT:
		{
			ShowMessage(L"О проекте", L"Fediska Injector v5.6\n\n", true);
		}
		break;
		}
		break;

	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;

	case WM_DESTROY:
		KillTimer(hwnd, TIMER_ID_HOVER);
		KillTimer(hwnd, TIMER_ID_STATUS);
		KillTimer(hwnd, TIMER_ID_ANIMATION);
		UnregisterHotKey(hwnd, 1);

		SaveSettings();

		if (g_hFont) DeleteObject(g_hFont);
		if (g_hDarkBrush) DeleteObject(g_hDarkBrush);
		if (g_hGameIcon) DestroyIcon(g_hGameIcon);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}



// Главная точка входа для Windows-приложения (wWinMain)

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow)
{
	WNDCLASSEX wc;
	HWND hwnd;
	MSG Msg;

	g_hInst = hInstance;
	SetDebugPrivilege(TRUE);

	LoadSettings();

	g_hFont = CreateFont(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

	// 1. Регистрация класса окна
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;

	// Загружаем БОЛЬШУЮ иконку из ресурсов
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = g_szClassName;
	//Загружаем МАЛЕНЬКУЮ иконку из ресурсов
	wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"Ошибка регистрации класса окна!", L"FediskaInjector", MB_ICONERROR | MB_OK);
		return 0;
	}

	// 2. Создание окна
	hwnd = CreateWindowEx(
		0,
		g_szClassName,
		g_szDefaultTitle,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 580, 260, // Высота подогнана под дизайн
		NULL, NULL, hInstance, NULL);

	if (hwnd == NULL)
	{
		MessageBox(NULL, L"Ошибка создания окна!", L"FediskaInjector", MB_ICONERROR | MB_OK);
		return 0;
	}

	// 3. Отображение и цикл сообщений
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	// 4. РЕГИСТРАЦИЯ ГОРЯЧЕЙ КЛАВИШИ (Используем загруженный g_hotKeyVk)
	RegisterHotKey(hwnd, 1, g_hotKeyMod, g_hotKeyVk);

	while (GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return (int)Msg.wParam;
}

// КОД Я ПИСАЛ ВЕСЬ САМ БЕЗ ИИ (Максимум, с чем мне помог ИИ это в написании комментариев в самом коде. Потому что самому
//Это писать очень долго, ну и мне было просто лень :D