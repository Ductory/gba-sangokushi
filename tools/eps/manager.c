#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shobjidl.h>
#include "resource.h"

#include <stdio.h>
#include <locale.h>

#include "manager.h"
#include "eps.h"
#include "core/gba.h"
#include "core/duckwin.h"
#include "core/encoding.h"
#include "utils/json.h"
#include "utils/io.h"
#include "utils/logger.h"
#include "windef.h"
#include "winnt.h"

#define COM_CHECK(hr,label) if (FAILED(hr)) goto label

static HINSTANCE Manager_hInstance;
static HWND Manager_hDlg;

static bool ROM_Modified, Config_Modified;
static bool Enable_Patching = true, Init_Patching;
static jobj_t Config;

static void manager_lvw_init(HWND hLvw)
{
	LVCOLUMN lvc = {
		.mask = LVCF_TEXT | LVCF_WIDTH,
		.pszText = TEXT("补丁"),
		.cx = 100
	};
	ListView_InsertColumn(hLvw, 0, &lvc);
	lvc.pszText = TEXT("描述");
	lvc.cx = 300;
	ListView_InsertColumn(hLvw, 1, &lvc);
}

static int lvw_addItem(HWND hLvw, LPTSTR patch, LPTSTR desc)
{
	int index = ListView_GetItemCount(hLvw);
	LVITEM lvi = {
		.mask = LVIF_TEXT,
		.iItem = index,
		.iSubItem = 0,
		.pszText = patch
	};
	ListView_InsertItem(hLvw, &lvi);
	lvi.iSubItem = 1;
	lvi.pszText = desc;
	ListView_SetItem(hLvw, &lvi);
	return index;
}

static void lvw_onChange(HWND hLvw, int index)
{
	if (!Enable_Patching)
		return;
	TCHAR tbuf[MAX_PATH];
	ListView_GetItemText(hLvw, index, 0, tbuf, lenof(tbuf));
	char mbuf[MAX_PATH];
	tcstombs(mbuf, tbuf, lenof(mbuf), CP_UTF8);

	u8 *patch;
	u32 size;
	if (!readfile(mbuf, &patch, &size)) {
		MessageBox(NULL, TEXT("无法打开补丁文件"), TEXT("操作失败"), MB_OK | MB_ICONERROR);
		return;
	}
	jitem_t item = json_get(Config, mbuf);
	u32 result = eps_apply(&ROM, &ROM_size, patch, size);
	if (result == 0 || result == 1) {
		item->b = result;
		ROM_Modified = true;
		Config_Modified = true;
	} else {
		if (result == -1) {
			MessageBox(NULL, TEXT("补丁损坏"), TEXT("操作失败"), MB_OK | MB_ICONERROR);
		}
		if (result == 2) {
			MessageBox(NULL, TEXT("无法应用补丁"), TEXT("操作失败"), MB_OK | MB_ICONERROR);
			eps_apply(&ROM, &ROM_size, patch, size); // 清除修改
		}
		ListView_SetCheckState(hLvw, index, FALSE);
	}
	free(patch);
}

static wchar_t *manager_open_ROM(void)
{
	static const COMDLG_FILTERSPEC cf[] = {
		{.pszName = L"GBA Files", .pszSpec = L"*.gba"},
		{.pszName = L"All Files", .pszSpec = L"*"}
	};
	return DwOpenFileDialog(Manager_hDlg, cf, lenof(cf));
}

static bool manager_load_ROM(const wchar_t *name)
{
	char buf[MAX_PATH];
	wcstombs(buf, name, lenof(buf));
	if (!load_ROM(buf))
		return false;
	return true;
}

static void manager_save_ROM(const char *name)
{
	if (!ROM || !name || !strlen(name))
		return;
	write_ROM(name);
}

static void manager_save_config(const char *name)
{
	FILE *fp = fopen(name, "w");
	fputs(json_save(Config, true), fp);
	fclose(fp);
}

static wchar_t *manager_open_patch(void)
{
	static const COMDLG_FILTERSPEC cf[] = {
		{.pszName = L"EPS Patch Files", .pszSpec = L"*.eps"},
		{.pszName = L"All Files", .pszSpec = L"*"}
	};
	return DwOpenFileDialog(Manager_hDlg, cf, lenof(cf));
}

static void add_patch(HWND hLvw, wchar_t *name, int checked)
{
	// 补丁名
	char mbuf[MAX_PATH];
	TCHAR tbuf[MAX_PATH];
	wcstombs(mbuf, name, lenof(mbuf));
	wcstotcs(tbuf, name, lenof(tbuf));

	// 打开补丁
	u8 *patch;
	u32 size;
	readfile(mbuf, &patch, &size);

	if (!Init_Patching) {
		if (Config && json_get(Config, mbuf)) // 已存在该补丁
			return;
		checked = eps_check(&ROM, &ROM_size, patch, size);
		if (checked != 0 && checked != 1) {
			if (checked == -1)
				MessageBox(Manager_hDlg, TEXT("补丁损坏"), TEXT("操作失败"), MB_OK | MB_ICONERROR);
			if (checked == 2)
				MessageBox(Manager_hDlg, TEXT("无法应用补丁"), TEXT("操作失败"), MB_OK | MB_ICONERROR);
			free(patch);
			return;
		}
		json_add(Config, mbuf, &(struct _jsonval){.t = JT_BOOL, .b = checked}); // 添加补丁到配置
	}
	// 补丁描述
	char *mdesc = eps_get_desc(patch, size);
	free(patch);

	size_t n = strlen(mdesc) + 1;
	TCHAR *tdesc = malloc(n * sizeof(TCHAR));
	mbstotcs(tdesc, mdesc, n, CP_UTF8);
	free(mdesc);
	// 添加到列表
	Enable_Patching = false; // 防止刷新列表时触发应用补丁
	int index = lvw_addItem(hLvw, tbuf, tdesc);
	free(tdesc);
	ListView_SetCheckState(hLvw, index, checked);
	Enable_Patching = true;

	Config_Modified = true;
not_available:
}

static void del_patch(HWND hLvw, int index)
{
	TCHAR tbuf[MAX_PATH];
	ListView_GetItemText(hLvw, index, 0, tbuf, lenof(tbuf));
	ListView_DeleteItem(hLvw, index);
	char mbuf[MAX_PATH];
	tcstombs(mbuf, tbuf, lenof(mbuf), CP_UTF8);
	json_rmv(Config, mbuf);
	Config_Modified = true;
}

static void init_eps_list(HWND hLvw)
{
	if (!Config)
		return;
	wchar_t wbuf[MAX_PATH];
	Init_Patching = true;
	JSON_FOR_OBJ(Config, item) {
		mbstowcs(wbuf, item->key, lenof(wbuf));
		add_patch(hLvw, wbuf, item->b);
	}
	Init_Patching = false;
}

static void manager_open_config(HWND hList, char *config_name, const char *rom_name)
{
	strcpy(config_name, rom_name);
	char *p = strrchr(config_name, '.');
	char *q = strrchr(config_name, '\\');
	if (q < p) // 存在扩展名
		*p = '\0';
	strcat_s(config_name, MAX_PATH, ".json");

	if (Config)
		json_free(Config);
	Config = json_loadf(config_name);
	if (!Config)
		Config = json_load("{}");
	init_eps_list(hList);
}

static void manager_open_file(HWND hList, char *rom_name, char *config_name)
{
	wchar_t *name = manager_open_ROM();
	if (!name || !manager_load_ROM(name)) { // 打开失败
		free(name);
		return;
	}
	wcstombs(rom_name, name, MAX_PATH);
	manager_open_config(hList, config_name, rom_name);
}

INT_PTR CALLBACK dlgproc_manager(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static char ROM_Name[MAX_PATH], Config_Name[MAX_PATH];
	static HWND hlvwEps;
	switch (uMsg) {
		case WM_INITDIALOG: {
			Manager_hDlg = hDlg;
			// init ListView
			hlvwEps = GetDlgItem(hDlg, IDC_LVWEPS);
			ListView_SetExtendedListViewStyle(hlvwEps, LVS_EX_CHECKBOXES);
			manager_lvw_init(hlvwEps);
			// init menu
			SetMenu(hDlg, LoadMenu(Manager_hInstance, MAKEINTRESOURCE(IDM_MANAGER)));
			// init dragdrop
			DragAcceptFiles(hDlg, TRUE);
			return TRUE;
		}
		case WM_CLOSE: {
			if (ROM_Modified) {
				int ret = MessageBox(hDlg, TEXT("ROM已修改。要保存吗？"), TEXT("保存"), MB_YESNO);
				if (ret == IDYES) {
					manager_save_ROM(ROM_Name);
					if (Config_Modified)
						manager_save_config(Config_Name);
				}
			}
			EndDialog(hDlg, wParam);
			return TRUE;
		}
		case WM_SIZE: {
			int w = LOWORD(lParam), h = HIWORD(lParam);
			SetWindowPos(hlvwEps, NULL, 0, 0, w, h, SWP_NOZORDER);
			return TRUE;
		}
		case WM_DROPFILES: {
			HDROP hDrop = (HDROP)wParam;
			TCHAR tbuf[MAX_PATH];
			wchar_t wbuf[MAX_PATH];
			if (DragQueryFile(hDrop, 0, tbuf, lenof(tbuf))) {
				tcstowcs(wbuf, tbuf, lenof(wbuf));
				if (!Config) {
					TCHAR title[MAX_PATH + 16];
					_tsprintf(title, lenof(title), TEXT("EPS Manager - %s"), tbuf);
					SetWindowText(hDlg, title);
					tcstombs(ROM_Name, tbuf, lenof(ROM_Name), CP_UTF8);
					manager_load_ROM(wbuf);
					manager_open_config(hlvwEps, Config_Name, ROM_Name);
				} else {
					add_patch(hlvwEps, wbuf, false);
				}
			}
			DragFinish(hDrop);
			return TRUE;
		}
		case WM_COMMAND: {
			switch (LOWORD(wParam)) {
				case IDI_FILE_OPEN: {
					manager_open_file(hlvwEps, ROM_Name, Config_Name);
					break;
				}
				case IDI_FILE_SAVE: {
					if (ROM_Modified) {
						manager_save_ROM(ROM_Name);
						ROM_Modified = false;
					}
					if (Config_Modified) {
						manager_save_config(Config_Name);
						Config_Modified = false;
					}
					break;
				}
				case IDI_ADD_PATCH: {
					wchar_t *name = manager_open_patch();
					add_patch(hlvwEps, name, false);
					free(name);
					break;
				}
				case IDI_DEL_PATCH: {
					int index = ListView_GetNextItem(hlvwEps, -1, LVNI_SELECTED);
					if (index >= 0)
						del_patch(hlvwEps, index);
					break;
				}
			}
			return TRUE;
		}
		case WM_NOTIFY: {
			LPNMHDR lpnmh = (LPNMHDR)lParam;
			if (lpnmh->idFrom == IDC_LVWEPS && lpnmh->code == LVN_ITEMCHANGED) {
				NMLISTVIEW *pnmv = (NMLISTVIEW*)lParam;
				if ((pnmv->uChanged & LVIF_STATE) && (pnmv->uNewState & LVIS_STATEIMAGEMASK) != (pnmv->uOldState & LVIS_STATEIMAGEMASK))
					lvw_onChange(pnmv->hdr.hwndFrom, pnmv->iItem);
			}
			return TRUE;
		}
		case WM_CONTEXTMENU: {
			if ((HWND)wParam == hlvwEps && Config) { // ListView 右键菜单 (仅当配置已加载)
				HMENU hMenu = LoadMenu(Manager_hInstance, MAKEINTRESOURCE(IDM_LIST));
				if (hMenu) {
					HMENU hSub = GetSubMenu(hMenu, 0);
					if (hSub) {
						POINT pt = {.x = LOWORD(lParam), .y = HIWORD(lParam)};
						TrackPopupMenu(hSub, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hDlg, NULL);	
					}
					DestroyMenu(hMenu);
				}
			}
			return TRUE;
		}
	}
	return FALSE;
}

int handler(...)
{
	MessageBox(NULL, TEXT("FATAL ERROR"), TEXT("ERROR"), MB_OK);
	PostQuitMessage(1);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	__try1(handler);
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	setlocale(LC_CTYPE, LC_UTF8);

	Manager_hInstance = hInstance;
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_MANAGER), NULL, dlgproc_manager);

	CoUninitialize();
	__except1;
	return 0;
}
