#ifndef _DUCKWIN_H
#define _DUCKWIN_H

#include "encoding.h"
#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <shobjidl.h>
#include <tchar.h>


#ifdef _UNICODE
#define mbstotcs(dest,src,max,cp) mbs2wcs(dest, src, -1, max, cp)
#define tcstombs(dest,src,max,cp) wcs2mbs(dest, src, -1, max, cp)
#define wcstotcs(dest,src,max) wcsncpy(dest, src, max)
#define tcstowcs(dest,src,max) wcsncpy(dest, src, max)
#define _tsprintf(stream,count,format,...) swprintf(stream, count, format, ## __VA_ARGS__)
#else
#define mbstotcs(dest,src,max,cp) mbs2mbs(dest, src, -1, max, CP_ACP, cp)
#define tcstombs(dest,src,max,cp) mbs2mbs(dest, src, -1, max, cp, CP_ACP)
#define wcstotcs(dest,src,max) wcs2ansi(dest, src, max)
#define tcstowcs(dest,src,max) ansi2wcs(dest, src, max)
#define _tsprintf(stream,count,format,...) sprintf(stream, format, ## __VA_ARGS__)
#endif

void DwAllocConsole(void);
DWORD DwGetProcessId(LPCTSTR proc_name);
LPVOID DwGetProcessBaseAddress(HANDLE hProcess);
DWORD DwGetProcessMainThreadId(DWORD dwPid);
BOOL DwSetHardwareBreakpoint(HANDLE hThread, int slot, LPVOID address, int type, int len);
BOOL DwResetHardwareBreakpoint(HANDLE hThread, int slot);
wchar_t *DwOpenFileDialog(HWND hwndOwner, const COMDLG_FILTERSPEC *cf, UINT num);
void HexListBox_SetString(HWND hList, int index, LPCTSTR lpsz);
void HexListBox_GetText(HWND hList, int index, TCHAR *buf);
void HexListBox_LoadStrings(HWND hList, const LPCTSTR *strs, size_t n);
void HexComboBox_LoadStrings(HWND hCombo, const LPCTSTR *strs, size_t n);
void ListBox_SetString(HWND hList, int index, LPCTSTR lpsz);
void ListBox_Clear(HWND hList);
void ComboBox_Clear(HWND hCombo);
void Static_SetImage(HWND hImg, GpImage *img);
void ShowControls(HWND hDlg, const int *list, size_t n, int show);
INT_PTR DefProc_RadioButton(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, int id_first, int id_last);

#endif // _DUCKWIN_H
