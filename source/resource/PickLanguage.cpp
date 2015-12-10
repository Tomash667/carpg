#include "Pch.h"
#include "Base.h"
#include "Language.h"
#pragma warning (disable: 4005)
#include "resource1.h"

//-----------------------------------------------------------------------------
struct ListItem
{
	string text, id;
};

//-----------------------------------------------------------------------------
static int dialog_state, dialog_result;
static vector<ListItem*> dialog_items;

//-----------------------------------------------------------------------------
void UpdateTexts(HWND hwndDlg, int select_index);

//=================================================================================================
static INT_PTR CALLBACK PickLanguageDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			HWND list = GetDlgItem(hwndDlg, IDC_LIST2);
			LCID lang = GetUserDefaultLCID();

			int index = 0, best_id = -1, best_dist, default_id = -1;
			Tokenizer t;

			// szukaj plików z informacj¹ o jêzyku
			for(vector<LanguageMap*>::iterator it = g_languages.begin(), end = g_languages.end(); it != end; ++it, ++index)
			{
				LanguageMap& lmap = **it;

				// parse locale
				t.FromString(lmap["locale"]);
				try
				{
					int dist = 0;
					while(t.Next())
					{
						int locale = t.MustGetInt();
						if(locale == lang && (best_id == -1 || best_dist < dist))
						{
							best_id = index;
							best_dist = dist;
						}
						t.Next();
						if(t.IsEof())
							break;
						t.AssertSymbol(';');
					}
				}
				catch(const Tokenizer::Exception& e)
				{
					ERROR(Format("Failed to parse locale for language '%s': %s", lmap["dir"].c_str(), e.ToString()));
				}

				// check if english for default
				const string& dir = lmap["dir"];
				if(dir == "en" && default_id == -1)
					default_id = index;

				// add to list				
				ListItem* item = new ListItem;
				item->id = dir;
				item->text = Format("%s, %s, %s", dir.c_str(), lmap["englishName"].c_str(), lmap["localName"].c_str());
				dialog_items.push_back(item);
				SendMessage(list, LB_ADDSTRING, 0, (LPARAM)item->text.c_str());
			}

			// select something
			int select_index;
			if(best_id != -1)
				select_index = best_id;
			else if(default_id != -1)
				select_index = default_id;
			else
				select_index = 0;
			SendMessage(list, LB_SETCURSEL, select_index, 0);

			UpdateTexts(hwndDlg, select_index);
		}
		return TRUE;

	case WM_CLOSE:
		DestroyWindow(hwndDlg);
		return TRUE;

	case WM_DESTROY:
		dialog_state = 2;
		return TRUE;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_LIST2:
			switch(HIWORD(wParam))
			{
			case LBN_DBLCLK:
				dialog_result = SendDlgItemMessage(hwndDlg, IDC_LIST2, LB_GETCURSEL, 0, 0);
				DestroyWindow(hwndDlg);
				return TRUE;
			case LBN_SELCHANGE:
				UpdateTexts(hwndDlg, SendMessage((HWND)lParam, LB_GETCURSEL, 0, 0));
				break;
			}
			break;
		case IDOK:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				dialog_result = SendDlgItemMessage(hwndDlg, IDC_LIST2, LB_GETCURSEL, 0, 0);
				DestroyWindow(hwndDlg);
				return TRUE;
			}
			break;
		case IDCANCEL:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				DestroyWindow(hwndDlg);
				return TRUE;
			}
			break;
		}
	}
	return FALSE;
}

//=================================================================================================
bool ShowPickLanguageDialog(string& lang)
{
	dialog_result = -1;
	dialog_state = 0;

	if(g_languages.empty())
	{
		MessageBox(nullptr, "Failed to load languaged data files!", "Language error", MB_APPLMODAL);
		return false;
	}

	HWND hwnd = CreateDialog(nullptr, MAKEINTRESOURCE(IDD_DIALOG1), nullptr, PickLanguageDialogProc);
	ShowWindow(hwnd, SW_SHOW);

	MSG aMsg = {};
	while(dialog_state == 0 && GetMessage(&aMsg, nullptr, 0, 0))
	{
		if(!IsDialogMessage(hwnd, &aMsg))
		{
			TranslateMessage(&aMsg);
			DispatchMessage(&aMsg);
		}
	}

	if(dialog_result == -1)
	{
		DeleteElements(dialog_items);
		return false;
	}
	else
	{
		lang = dialog_items[dialog_result]->id;
		DeleteElements(dialog_items);
		return true;
	}
}

//=================================================================================================
static void UpdateTexts(HWND hwndDlg, int select_index)
{
	// update texts
	LanguageMap& lmap = *g_languages[select_index];
	LanguageMap::iterator it = lmap.find("text"), end = lmap.end();
	if(it != end)
		SetDlgItemText(hwndDlg, IDC_STATIC1, it->second.c_str());
	it = lmap.find("title");
	if(it != end)
		SetWindowText(hwndDlg, it->second.c_str());
	it = lmap.find("bOk");
	if(it != end)
		SetDlgItemText(hwndDlg, IDOK, it->second.c_str());
	it = lmap.find("bCancel");
	if(it != end)
		SetDlgItemText(hwndDlg, IDCANCEL, it->second.c_str());
}
