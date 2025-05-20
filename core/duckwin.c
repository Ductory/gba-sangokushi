#include "duckwin.h"
#include "gdiplus/gdiplusgpstubs.h"
#include "utils/logger.h"
#include "windows.h"

#include <tlhelp32.h>
#include <psapi.h>

#define BUF_MAX 2048
#define COM_CHECK(hr,label) if (FAILED(hr)) goto label


void DwAllocConsole(void)
{
    AllocConsole();
    freopen("CONIN$", "r+t", stdin);
    freopen("CONOUT$", "w+t", stdout);
    freopen("CONOUT$", "w+t", stderr);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

DWORD DwGetProcessId(LPCTSTR proc_name)
{
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            if (_tcscmp(entry.szExeFile, proc_name) == 0)
            {
                return entry.th32ProcessID;
            }
        }
    }
    return 0;
}

LPVOID DwGetProcessBaseAddress(HANDLE hProcess)
{
    LPVOID baseAddress = 0;
    HMODULE hMod;
    DWORD cbNeeded;
    if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
        MODULEINFO modInfo;
        if (GetModuleInformation(hProcess, hMod, &modInfo, sizeof(modInfo)))
            baseAddress = modInfo.lpBaseOfDll;
    }
    return baseAddress;
}

DWORD DwGetProcessMainThreadId(DWORD dwPid)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, dwPid);
    THREADENTRY32 entry;
    entry.dwSize = sizeof(THREADENTRY32);
    DWORD mainThreadId;
    if (!Thread32First(hSnapshot, &entry))
        return 0;
    do {
        if (entry.th32OwnerProcessID == dwPid) {
            mainThreadId = entry.th32ThreadID;
            break;
        }
    } while (Thread32Next(hSnapshot, &entry));
    return mainThreadId;
}

BOOL DwSetHardwareBreakpoint(HANDLE hThread, int slot, LPVOID address, int type, int len)
{
    if (slot < 0 || slot >= 4)
        return FALSE;
    CONTEXT ctx = {.ContextFlags = CONTEXT_DEBUG_REGISTERS};
    SuspendThread(hThread);
    GetThreadContext(hThread, &ctx);
    switch (slot) {
        case 0: ctx.Dr0 = (DWORD_PTR)address; break;
        case 1: ctx.Dr1 = (DWORD_PTR)address; break;
        case 2: ctx.Dr2 = (DWORD_PTR)address; break;
        case 3: ctx.Dr3 = (DWORD_PTR)address; break;
    }
    DWORD64 condition = (type & 0x3) | ((len & 0x3) << 2);
    ctx.Dr7 |= 1 << (slot * 2) | condition << (16 + slot * 4);
    BOOL br = SetThreadContext(hThread, &ctx);
    ResumeThread(hThread);
    return br;
}

BOOL DwResetHardwareBreakpoint(HANDLE hThread, int slot)
{
    if (slot < 0 || slot >= 4)
        return FALSE;
    CONTEXT ctx = {.ContextFlags = CONTEXT_DEBUG_REGISTERS};
    SuspendThread(hThread);
    GetThreadContext(hThread, &ctx);
    switch (slot) {
        case 0: ctx.Dr0 = 0; break;
        case 1: ctx.Dr1 = 0; break;
        case 2: ctx.Dr2 = 0; break;
        case 3: ctx.Dr3 = 0; break;
    }
    ctx.Dr7 &= ~(1 << (slot * 2) | 0xF << (16 + slot * 4));
    BOOL br = SetThreadContext(hThread, &ctx);
    ResumeThread(hThread);
    return br;
}

wchar_t *DwOpenFileDialog(HWND hwndOwner, const COMDLG_FILTERSPEC *cf, UINT num)
{
    wchar_t *result = NULL;
    HRESULT hr;
    IFileOpenDialog *pFileOpen;
    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_ALL, &IID_IFileOpenDialog, (void**)&pFileOpen);
    COM_CHECK(hr, CreateInstance_fail);
    hr = pFileOpen->lpVtbl->SetFileTypes(pFileOpen, num, cf);
    COM_CHECK(hr, SetFileTypes_fail);
    hr = pFileOpen->lpVtbl->Show(pFileOpen, hwndOwner);
    COM_CHECK(hr, Show_fail);

    IShellItem *pItem;
    hr = pFileOpen->lpVtbl->GetResult(pFileOpen, &pItem);
    COM_CHECK(hr, GetResult_fail);

    wchar_t *path;
    hr = pItem->lpVtbl->GetDisplayName(pItem, SIGDN_FILESYSPATH, &path);
    COM_CHECK(hr, GetDisplayName_fail);
    if (path && wcslen(path) && GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES) // check if file exists
        result = wcsdup(path);
    CoTaskMemFree(path);

GetDisplayName_fail:
    pItem->lpVtbl->Release(pItem);
GetResult_fail:
Show_fail:
SetFileTypes_fail:
    pFileOpen->lpVtbl->Release(pFileOpen);
CreateInstance_fail:
    return result;
}

static int get_hex_len(size_t n)
{
    int len = 0;
    for (; n; ++len, n >>= 4);
    return len;
}

void HexListBox_SetString(HWND hList, int index, LPCTSTR lpsz)
{
    int len = get_hex_len(ListBox_GetCount(hList) - 1);
    TCHAR buf[BUF_MAX];
    if (_tcslen(lpsz) >= BUF_MAX) {
        LOG_E("string is too long");
        return;
    }
    _tsprintf(buf, lenof(buf), TEXT("%0*X:%s"), len, index, lpsz);
    ListBox_SetString(hList, index, buf);
}


void HexListBox_GetText(HWND hList, int index, TCHAR *buf)
{
    int len = get_hex_len(ListBox_GetCount(hList) - 1);
    LRESULT lr = ListBox_GetText(hList, index, buf);
    if (lr == LB_ERR)
        return;
    _tcscpy(buf, buf + len + 1); // skip prefix
}

void HexListBox_LoadStrings(HWND hList, const LPCTSTR *strs, size_t n)
{
    ListBox_Clear(hList);
    TCHAR buf[BUF_MAX];
    int len = get_hex_len(n - 1);
    for (size_t i = 0; i < n; ++i) {
        _tsprintf(buf, lenof(buf), TEXT("%0*zX:%s"), len, i, strs[i]);
        ListBox_AddString(hList, buf);
    }
}

void HexComboBox_LoadStrings(HWND hCombo, const LPCTSTR *strs, size_t n)
{
    ComboBox_Clear(hCombo);
    TCHAR buf[BUF_MAX];
    int len = get_hex_len(n);
    for (size_t i = 0; i < n; ++i) {
        _tsprintf(buf, lenof(buf), TEXT("%0*zX:%s"), len, i, strs[i]);
        ComboBox_AddString(hCombo, buf);
    }
}

void ListBox_SetString(HWND hList, int index, LPCTSTR lpsz)
{
    if (!hList || index == LB_ERR)
        return;
    ListBox_DeleteString(hList, index);
    ListBox_InsertString(hList, index, lpsz);
}

void ListBox_Clear(HWND hList)
{
    while (ListBox_DeleteString(hList, 0) > 0);
}

void ComboBox_Clear(HWND hCombo)
{
    while (ComboBox_DeleteString(hCombo, 0) > 0);
}

void Static_SetImage(HWND hImg, GpImage *img)
{
    HBITMAP hBmp;
    GdipCreateHBITMAPFromBitmap(img, &hBmp, 0);
    DeleteObject((HGDIOBJ)SendMessage(hImg, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmp));
}

void ShowControls(HWND hDlg, const int *list, size_t n, int show)
{
    for (size_t i = 0; i < n; ++i)
        ShowWindow(GetDlgItem(hDlg, list[i]), show);
}

INT_PTR DefProc_RadioButton(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, int id_first, int id_last)
{
    switch (uMsg) {
        case WM_INITDIALOG: {
            CheckRadioButton(hDlg, id_first, id_last, LOWORD(wParam));
            return TRUE;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) >= id_first && LOWORD(wParam) <= id_last) {
                switch (HIWORD(wParam)) {
                    case BN_CLICKED: {
                        CheckRadioButton(hDlg, id_first, id_last, LOWORD(wParam));
                        return TRUE;
                    }
                }
            }
            break;
        }
    }
    return FALSE;
}
