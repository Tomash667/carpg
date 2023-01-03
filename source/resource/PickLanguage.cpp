#include "Pch.h"
#include "Tokenizer.h"
#include "Language.h"
#pragma warning (disable: 4005)
#include "resource1.h"

//-----------------------------------------------------------------------------
struct ListItem
{
	string text, id;
};

//-----------------------------------------------------------------------------
static int dialogState, dialogResult;
static vector<ListItem*> dialogItems;

//-----------------------------------------------------------------------------
void UpdateTexts(HWND hwndDlg, int selectIndex);

//=================================================================================================
static INT_PTR CALLBACK PickLanguageDialogProc(HWND hwndDlg, uint uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			HWND list = GetDlgItem(hwndDlg, IDC_LIST2);
			LCID lang = GetUserDefaultLCID();

			int index = 0, bestId = -1, bestDist, defaultId = -1;
			Tokenizer t;

			// szukaj plików z informacj¹ o jêzyku
			for(Language::Map* pLmap : Language::GetLanguages())
			{
				Language::Map& lmap = *pLmap;

				// parse locale
				t.FromString(lmap["locale"]);
				try
				{
					int dist = 0;
					while(t.Next())
					{
						int locale = t.MustGetInt();
						if(locale == lang && (bestId == -1 || bestDist < dist))
						{
							bestId = index;
							bestDist = dist;
						}
						t.Next();
						if(t.IsEof())
							break;
						t.AssertSymbol(';');
					}
				}
				catch(const Tokenizer::Exception& e)
				{
					Error("Failed to parse locale for language '%s': %s", lmap["dir"].c_str(), e.ToString());
				}

				// check if english for default
				const string& dir = lmap["dir"];
				if(dir == "en" && defaultId == -1)
					defaultId = index;

				// add to list
				ListItem* item = new ListItem;
				item->id = dir;
				item->text = Format("%s, %s, %s", dir.c_str(), lmap["englishName"].c_str(), lmap["localName"].c_str());
				dialogItems.push_back(item);
				SendMessage(list, LB_ADDSTRING, 0, (LPARAM)item->text.c_str());

				++index;
			}

			// select something
			int selectIndex;
			if(bestId != -1)
				selectIndex = bestId;
			else if(defaultId != -1)
				selectIndex = defaultId;
			else
				selectIndex = 0;
			SendMessage(list, LB_SETCURSEL, selectIndex, 0);

			UpdateTexts(hwndDlg, selectIndex);
		}
		return TRUE;

	case WM_CLOSE:
		DestroyWindow(hwndDlg);
		return TRUE;

	case WM_DESTROY:
		dialogState = 2;
		return TRUE;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_LIST2:
			switch(HIWORD(wParam))
			{
			case LBN_DBLCLK:
				dialogResult = SendDlgItemMessage(hwndDlg, IDC_LIST2, LB_GETCURSEL, 0, 0);
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
				dialogResult = SendDlgItemMessage(hwndDlg, IDC_LIST2, LB_GETCURSEL, 0, 0);
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
	dialogResult = -1;
	dialogState = 0;

	if(Language::GetLanguages().empty())
	{
		MessageBox(nullptr, "Failed to load languaged data files!", "Language error", MB_APPLMODAL);
		return false;
	}

	HWND hwnd = CreateDialog(nullptr, MAKEINTRESOURCE(IDD_DIALOG1), nullptr, PickLanguageDialogProc);
	ShowWindow(hwnd, SW_SHOW);

	MSG aMsg = {};
	while(dialogState == 0 && GetMessage(&aMsg, nullptr, 0, 0))
	{
		if(!IsDialogMessage(hwnd, &aMsg))
		{
			TranslateMessage(&aMsg);
			DispatchMessage(&aMsg);
		}
	}

	if(dialogResult == -1)
	{
		DeleteElements(dialogItems);
		return false;
	}
	else
	{
		lang = dialogItems[dialogResult]->id;
		DeleteElements(dialogItems);
		return true;
	}
}

//=================================================================================================
static void UpdateTexts(HWND hwndDlg, int selectIndex)
{
	// update texts
	Language::Map& lmap = *Language::GetLanguages()[selectIndex];
	auto it = lmap.find("text"), end = lmap.end();
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
