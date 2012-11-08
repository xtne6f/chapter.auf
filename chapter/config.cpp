//---------------------------------------------------------------------
//		プラグイン設定
//---------------------------------------------------------------------
#include <Windows.h>
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <cstdio>
#include <string>
#include <regex>
#include <emmintrin.h>
#include "resource.h"
#include "config.h"
#include "mylib.h"

//[ru]計測クラス
//#define CHECKSPEED
#ifdef CHECKSPEED
class QPC {
	LARGE_INTEGER freq;
	LARGE_INTEGER diff;
	LARGE_INTEGER countstart;
public:
	QPC() {
		QueryPerformanceFrequency( &freq );
		diff.QuadPart = 0;
	}

	void start() {
		QueryPerformanceCounter( &countstart );
	}
	void stop() {
		LARGE_INTEGER countend;
		QueryPerformanceCounter( &countend );
		diff.QuadPart += countend.QuadPart - countstart.QuadPart;
	}

	void reset() {
		diff.QuadPart = 0;
	}

	double get() {
		return (double)(diff.QuadPart) / freq.QuadPart * 1000.;
	}
};
#else
class QPC {
public:
	QPC() {}

	void start() {}
	void stop() {}

	void reset() {}

	double get() { return 0.0;}
};
#endif
//ここまで

void CfgDlg::Init(HWND hwnd,void *editp,FILTER *fp) {
	HFONT hfont,hfont2;
	char str[STRLEN];
	HINSTANCE hinst = fp->dll_hinst;

	m_fp = fp;
	m_exfunc = fp->exfunc;
	m_scale = 30000;	// 29.97fps
	m_rate = 1001;	// 29.97fps
	m_numFrame = 0;
	m_numChapter = 0;
	m_numHis = 0;
	m_editp = NULL;
	m_hDlg = hwnd;
	m_loadfile = false;
	m_listBoxHeight = 332;
	m_timelineHeight = 0;
	m_thumbWidth = 0;
	m_thumbHeight = 0;
	m_thumbAspect = 90;
	m_thumbsNum = 7;
	ZeroMemory(m_hbmThumbs, sizeof(m_hbmThumbs));

	m_numChapterCapacity = 0;
	m_Frame = NULL;
	m_strTitle = NULL;
	ReserveChapterList(1);

	// フォント
	hfont2 = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	hfont = my_getfont(fp,editp);	// AviUtlデフォルトフォント

	// ウインドウの作成（部品追加）
	SendMessage(hwnd,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindowEx(WS_EX_CLIENTEDGE,"LISTBOX","",WS_CHILD|WS_VISIBLE|LBS_NOTIFY|WS_VSCROLL|WS_TABSTOP,0,0,448,335,hwnd,(HMENU)IDC_LIST1,hinst,0);
	SendDlgItemMessage(hwnd,IDC_LIST1,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",WS_CHILD|WS_VISIBLE|ES_READONLY,48,336,190,20,hwnd,(HMENU)IDC_EDTIME,hinst,0);
	SendDlgItemMessage(hwnd,IDC_EDTIME,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindowEx(WS_EX_CLIENTEDGE,"COMBOBOX","",WS_CHILD|WS_VISIBLE|CBS_DROPDOWN|WS_VSCROLL|WS_TABSTOP,48,357,400,120,hwnd,(HMENU)IDC_EDNAME,hinst,0);
	SendDlgItemMessage(hwnd,IDC_EDNAME,WM_SETFONT,(WPARAM)hfont2,0);
	CreateWindow("BUTTON","保存",WS_CHILD|WS_VISIBLE,450,12,73,22,hwnd,(HMENU)IDC_BUSAVE,hinst,0);
	SendDlgItemMessage(hwnd,IDC_BUSAVE,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("BUTTON","読込",WS_CHILD|WS_VISIBLE,450,40,73,22,hwnd,(HMENU)IDC_BULOAD,hinst,0);
	SendDlgItemMessage(hwnd,IDC_BULOAD,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("BUTTON","自動出力",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,450,290,73,22,hwnd,(HMENU)IDC_CHECK1,hinst,0);
	SendDlgItemMessage(hwnd,IDC_CHECK1,WM_SETFONT,(WPARAM)hfont,0);
	//[ru]ボタン追加
	CreateWindow("BUTTON","無音部分",WS_CHILD|WS_VISIBLE,450,68,73,22,hwnd,(HMENU)IDC_BUDETECT,hinst,0);
	SendDlgItemMessage(hwnd,IDC_BUDETECT,WM_SETFONT,(WPARAM)hfont,0);
	
	CreateWindow("BUTTON","無音削除",WS_CHILD|WS_VISIBLE,450,96,73,22,hwnd,(HMENU)IDC_BUDELMUTE,hinst,0);
	SendDlgItemMessage(hwnd,IDC_BUDELMUTE,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("BUTTON","編集点展開",WS_CHILD|WS_VISIBLE,450,124,73,22,hwnd,(HMENU)IDC_BUEXPANDEDIT,hinst,0);
	SendDlgItemMessage(hwnd,IDC_BUEXPANDEDIT,WM_SETFONT,(WPARAM)hfont,0);

	CreateWindow("STATIC","連続",WS_CHILD|WS_VISIBLE,450,130+25,73,22,hwnd,(HMENU)IDC_STATICa,hinst,0);
	SendDlgItemMessage(hwnd,IDC_STATICa,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",WS_CHILD|WS_VISIBLE,490,127+25,33,22,hwnd,(HMENU)IDC_EDITSERI,hinst,0);
	SendDlgItemMessage(hwnd,IDC_EDITSERI,WM_SETFONT,(WPARAM)hfont,0);

	CreateWindow("STATIC","閾値",WS_CHILD|WS_VISIBLE,450,160+25,73,22,hwnd,(HMENU)IDC_STATICb,hinst,0);
	SendDlgItemMessage(hwnd,IDC_STATICb,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",WS_CHILD|WS_VISIBLE,490,157+25,33,22,hwnd,(HMENU)IDC_EDITMUTE,hinst,0);
	SendDlgItemMessage(hwnd,IDC_EDITMUTE,WM_SETFONT,(WPARAM)hfont,0);
	
	CreateWindow("BUTTON","SC位置",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,450,215,73,22,hwnd,(HMENU)IDC_CHECKSC,hinst,0);
	SendDlgItemMessage(hwnd,IDC_CHECKSC,WM_SETFONT,(WPARAM)hfont,0);
	//--ここまで
	CreateWindow("BUTTON","全SC検出",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,450,240,90,22,hwnd,(HMENU)IDC_PRECHECK,hinst,0);
	SendDlgItemMessage(hwnd,IDC_PRECHECK,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("BUTTON","mark付与",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,450,265,90,22,hwnd,(HMENU)IDC_SCMARK,hinst,0);
	SendDlgItemMessage(hwnd,IDC_SCMARK,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("BUTTON","既定保存",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,450,315,90,22,hwnd,(HMENU)IDC_CHECKDEFSAVE,hinst,0);
	SendDlgItemMessage(hwnd,IDC_CHECKDEFSAVE,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,450,340,73,22,hwnd,(HMENU)IDC_EDFILEEXT,hinst,0);
	SendDlgItemMessage(hwnd,IDC_EDFILEEXT,WM_SETFONT,(WPARAM)hfont,0);

	for (int i = 0; i < NUMTHUMBS; i++) {
		CreateWindow("STATIC","",WS_CHILD|WS_VISIBLE|SS_BITMAP|SS_NOTIFY,0,335,0,0,hwnd,(HMENU)(IDC_THUMBS_MIN+i),hinst,0);
	}
	HWND hItem = CreateWindow("STATIC","",WS_CHILD|WS_VISIBLE|SS_NOTIFY/*|SS_BLACKFRAME*/,0,335+5,550,0,hwnd,(HMENU)IDC_TIMELINE,hinst,0);
	SendMessage(hItem,WM_SETFONT,(WPARAM)hfont2,0);
	// [xt]ウィンドウプロシージャを登録
	m_pStaticWndProc = (WNDPROC)GetWindowLong(hItem,GWL_WNDPROC);
	if (m_pStaticWndProc) {
		SetWindowLong(hItem,GWL_USERDATA,(LONG)this);
		SetWindowLong(hItem,GWL_WNDPROC,(LONG)TimelineWndProc);
		m_hoveredChapter = -1;
		m_mouseTracking = false;
	}


	CreateWindow("BUTTON","削除",WS_CHILD|WS_VISIBLE,450,335,73,22,hwnd,(HMENU)IDC_BUDEL,hinst,0);
	SendDlgItemMessage(hwnd,IDC_BUDEL,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("BUTTON","追加",WS_CHILD|WS_VISIBLE,450,360,73,22,hwnd,(HMENU)IDC_BUADD,hinst,0);
	SendDlgItemMessage(hwnd,IDC_BUADD,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("STATIC","時間",WS_CHILD|WS_VISIBLE,12,336,31,17,hwnd,(HMENU)IDC_STATIC1,hinst,0);
	SendDlgItemMessage(hwnd,IDC_STATIC1,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("STATIC","名称",WS_CHILD|WS_VISIBLE,12,360,31,17,hwnd,(HMENU)IDC_STATIC2,hinst,0);
	SendDlgItemMessage(hwnd,IDC_STATIC2,WM_SETFONT,(WPARAM)hfont,0);

	CreateWindow("BUTTON","サムネ",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|BS_PUSHLIKE,240,336,58,20,hwnd,(HMENU)IDC_CHECKTHUMBS,hinst,0);
	SendDlgItemMessage(hwnd,IDC_CHECKTHUMBS,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("BUTTON","ﾀｲﾑﾗｲﾝ",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|BS_PUSHLIKE,300,336,58,20,hwnd,(HMENU)IDC_CHECKTIMELINE,hinst,0);
	SendDlgItemMessage(hwnd,IDC_CHECKTIMELINE,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("STATIC","",WS_CHILD|WS_VISIBLE|SS_RIGHT,0,336,140,17,hwnd,(HMENU)IDC_STATICTIME,hinst,0);
	SendDlgItemMessage(hwnd,IDC_STATICTIME,WM_SETFONT,(WPARAM)hfont,0);

	// [xt]コントロールの配置に影響するのでここで設定を読む
	CheckDlgButton(hwnd, IDC_CHECKTIMELINE, m_exfunc->ini_load_int(fp, "timeline", 0));
	CheckDlgButton(hwnd, IDC_CHECKTHUMBS, m_exfunc->ini_load_int(fp, "thumbs", 0));
	CheckDlgButton(hwnd, IDC_CHECKDEFSAVE, m_exfunc->ini_load_int(fp, "defsave", 0));
	m_thumbAspect = m_exfunc->ini_load_int(fp, "thumbAspect", m_thumbAspect);
	m_thumbsNum = m_exfunc->ini_load_int(fp, "thumbsNum", m_thumbsNum);
	m_thumbsNum = min(max(m_thumbsNum, 1), NUMTHUMBS);
	char ext[STRLEN];
	m_exfunc->ini_load_str(fp, "extension", ext, "");
	if (ext[0] != '.') strcpy_s(ext, ".txt");
	SetDlgItemText(hwnd, IDC_EDFILEEXT, ext);

	Resize();

	// tooltip
	struct {
		int id;
		char *help;
	} tips[] = {
		{ IDC_BUSAVE, "チャプターファイル（*.txt）を保存します。" },
		{ IDC_BULOAD, "チャプターファイル（*.txt）を読み込みます。画面にファイルをD&Dしても読み込めます。" },
		{ IDC_CHECK1, "エンコード開始時に、チャプター一覧を chapter.txt に書き出します。無音検索では基本的に使いません。" },
		{ IDC_BUDEL, "選択したチャプター（無音位置）を一覧から削除します。" },
		{ IDC_BUADD, "チャプター（無音位置）を一覧に追加します。無音検索では基本的に使いません。" },
		{ IDC_BUDETECT, "無音位置の検索を開始します。" },
		{ IDC_EDITSERI, "検出する最低連続無音フレーム数（5 ～）デフォルト：10" },
		{ IDC_EDITMUTE, "無音と判定する閾値（0 ～ 32767）デフォルト：50" },
		{ IDC_CHECKSC, "“無音位置へシークする時”、シーンチェンジを検出してその位置を表示します。" },
		{ IDC_PRECHECK, "「無音検索」時に、シーンチェンジ検索も併せて行います（時間がかかります）。" },
		{ IDC_SCMARK, "「全SC検索」チェック時に無音検索した場合、または「読込」時にSCPos情報がある場合、シーンチェンジ位置にマークを打ちます。" },
		{ IDC_BUDELMUTE, "名称が\"{数値}フレーム～\"になっているチャプターを一覧からすべて削除します。" },
		{ IDC_BUEXPANDEDIT, "名称が\"編集点 (間隔：～\"になっているチャプターを展開して後続のチャプター位置を調整します。" },
		{ IDC_CHECKDEFSAVE, "「保存」時に\"動画ファイル名+チャプター拡張子\"のファイル名で一発保存します。" },
		{ IDC_EDFILEEXT, "チャプターファイルの拡張子を指定します。" },
		{ IDC_CHECKTIMELINE, "チャプター位置のタイムラインを表示します。" },
		{ IDC_CHECKTHUMBS, "前後フレームのサムネイルを表示します。左端サムネイルを左クリックでポップアップ設定が出ます。" },
	};

	TOOLINFO ti;
	ZeroMemory(&ti, sizeof(ti));
	HWND htip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, TTS_ALWAYSTIP | TTS_NOPREFIX | TTS_BALLOON, 
		CW_USEDEFAULT,CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hwnd, 0, hinst, 
		NULL);
	ti.cbSize = sizeof(ti);
	ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
	ti.hwnd = hwnd;
	for(int i=0; i<sizeof(tips)/sizeof(tips[0]); i++) {
		ti.uId = (UINT_PTR)GetDlgItem(hwnd, tips[i].id);
		ti.lpszText = tips[i].help;
		SendMessage(htip , TTM_ADDTOOL , 0 , (LPARAM)&ti);
	}

	// コンボボックスに履歴追加
	for (int n=0; n<NUMHIS; n++) {
		sprintf_s(str, "history%d", n);
		m_exfunc->ini_load_str(fp, str, m_strHis[n], NULL);
		if (m_strHis[n][0] != NULL) {
			m_numHis++;
		}
	}
	AddHis();

	// 自動出力のチェックボックス
	m_autosave = m_exfunc->ini_load_int(fp,"autosave",0);
	CheckDlgButton(hwnd, IDC_CHECK1, m_autosave);

	//[ru]設定を読み込む
	int seri = m_exfunc->ini_load_int(fp, "muteCount", 10);
	sprintf_s(str, STRLEN, "%d", seri);
	SetDlgItemText(hwnd, IDC_EDITSERI, str);

	int mute = m_exfunc->ini_load_int(fp, "muteLimit", 50);
	sprintf_s(str, STRLEN, "%d", mute);
	SetDlgItemText(hwnd, IDC_EDITMUTE, str);

	CheckDlgButton(hwnd, IDC_CHECKSC, m_exfunc->ini_load_int(fp,"sceneChange", 1));
	CheckDlgButton(hwnd, IDC_PRECHECK, m_exfunc->ini_load_int(fp,"PrecheckSC", 0));
	CheckDlgButton(hwnd, IDC_SCMARK, m_exfunc->ini_load_int(fp,"SCMark", 0));
	//ここまで
}

void CfgDlg::Exit() {
	// [xt]ウィンドウプロシージャを登録解除(WM_DESTROYでは手遅れ)
	if (m_pStaticWndProc) {
		SetWindowLong(GetDlgItem(m_hDlg, IDC_TIMELINE), GWL_WNDPROC, (LONG)m_pStaticWndProc);
		m_pStaticWndProc = NULL;
	}
	ClearThumbs();
	FreeChapterList();
	m_hDlg = NULL;
}

void CfgDlg::Resize() {
	if (!m_hDlg) {
		return;
	}

	RECT rc;
	GetClientRect(m_hDlg, &rc);
	int w = rc.right;
	int h = rc.bottom;

	int left = w - 80;
	int bottom = h - 55;

	GetWindowRect(GetDlgItem(m_hDlg, IDC_LIST1), &rc);
	int oldLeft = rc.right - rc.left;
	int oldTop = m_listBoxHeight + m_thumbHeight + m_timelineHeight;

	// [xt]リストボックスのリサイズに連動して下側コントロールが振動するので、高さを変数で保持するようにした
	m_thumbWidth = IsDlgButtonChecked(m_hDlg, IDC_CHECKTHUMBS) == BST_CHECKED ? w / m_thumbsNum - 1 : 0;
	m_thumbHeight = m_thumbWidth * m_thumbAspect / 160;
	m_timelineHeight = IsDlgButtonChecked(m_hDlg, IDC_CHECKTIMELINE) == BST_CHECKED ? 22 + 4 : 0;
	m_listBoxHeight = bottom - m_thumbHeight - m_timelineHeight;

	MoveWindow(GetDlgItem(m_hDlg, IDC_LIST1), 0, 0, left, m_listBoxHeight - 2, TRUE);
	for (int i = 0; i < NUMTHUMBS; i++) {
		MoveWindow(GetDlgItem(m_hDlg, IDC_THUMBS_MIN + i), i * (m_thumbWidth + 1), m_listBoxHeight, m_thumbWidth, m_thumbHeight, TRUE);
	}
	MoveWindow(GetDlgItem(m_hDlg, IDC_TIMELINE), 0, m_listBoxHeight + m_thumbHeight + 2, left, m_timelineHeight - 4, TRUE);

	// 右側
	int rightItems[] = {
		IDC_BUSAVE,
		IDC_BULOAD,
		IDC_CHECK1,
		IDC_BUDETECT,
		IDC_STATICa,
		IDC_EDITSERI,
		IDC_STATICb,
		IDC_EDITMUTE,
		IDC_CHECKSC,
		IDC_PRECHECK,
		IDC_SCMARK,
		IDC_BUDELMUTE,
		IDC_BUEXPANDEDIT,
		IDC_CHECKDEFSAVE,
		IDC_EDFILEEXT,
		IDC_BUDEL,
		IDC_BUADD,
		0
	};

	for (int i=0; rightItems[i]; ++i) {
		HWND hItem = GetDlgItem(m_hDlg, rightItems[i]);
		GetWindowRect(hItem, &rc);
		MapWindowPoints(NULL, m_hDlg, (LPPOINT)&rc, 2);
		MoveWindow(hItem, rc.left + left - oldLeft, rc.top, rc.right - rc.left, rc.bottom  - rc.top, TRUE);
		if (rightItems[i] != IDC_BUDEL && rightItems[i] != IDC_BUADD) {
			// [xt]ほかのコントロールとかぶるよりは非表示のほうが安全
			if (m_listBoxHeight < rc.bottom) {
				if (IsWindowVisible(hItem)) ShowWindow(hItem, SW_HIDE);
			} else {
				if (!IsWindowVisible(hItem)) ShowWindow(hItem, SW_SHOW);
			}
		}
	}

	// 下側
	int bottomItems[] = {
		IDC_CHECKTIMELINE,
		IDC_CHECKTHUMBS,
		IDC_STATICTIME,
		IDC_EDTIME,
		IDC_EDNAME,
		IDC_STATIC1,
		IDC_STATIC2,
		IDC_BUDEL,
		IDC_BUADD,
		0,
	};

	for (int i=0; bottomItems[i]; ++i) {
		HWND hItem = GetDlgItem(m_hDlg, bottomItems[i]);
		GetWindowRect(hItem, &rc);
		MapWindowPoints(NULL, m_hDlg, (LPPOINT)&rc, 2);
		MoveWindow(hItem, rc.left, rc.top + bottom - oldTop, rc.right - rc.left, rc.bottom  - rc.top, TRUE);
	}

	// 下側の横幅
	HWND hItem = GetDlgItem(m_hDlg, IDC_EDNAME);
	GetWindowRect(hItem, &rc);
	MapWindowPoints(NULL, m_hDlg, (LPPOINT)&rc, 2);
	MoveWindow(hItem, rc.left, rc.top, left - rc.left, rc.bottom - rc.top, TRUE);

	hItem = GetDlgItem(m_hDlg, IDC_TIMELINE);
	GetWindowRect(hItem, &rc);
	MapWindowPoints(NULL, m_hDlg, (LPPOINT)&rc, 2);
	MoveWindow(hItem, rc.left, rc.top, w - rc.left, rc.bottom - rc.top, TRUE);

	hItem = GetDlgItem(m_hDlg, IDC_STATICTIME);
	GetWindowRect(hItem, &rc);
	MapWindowPoints(NULL, m_hDlg, (LPPOINT)&rc, 2);
	MoveWindow(hItem, left-(rc.right-rc.left) < 360 ? -1000 : left-(rc.right-rc.left),
	           rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
}

void CfgDlg::AutoSaveCheck() {
	m_autosave = IsDlgButtonChecked(m_fp->hwnd,IDC_CHECK1);
	m_exfunc->ini_save_int(m_fp,"autosave",m_autosave);
	//[ru]保存
	m_exfunc->ini_save_int(m_fp,"sceneChange", IsDlgButtonChecked(m_fp->hwnd, IDC_CHECKSC));
	//ここまで
	m_exfunc->ini_save_int(m_fp,"PrecheckSC", IsDlgButtonChecked(m_fp->hwnd, IDC_PRECHECK));
	m_exfunc->ini_save_int(m_fp,"SCMark", IsDlgButtonChecked(m_fp->hwnd, IDC_SCMARK));
}

void CfgDlg::SetFps(int rate,int scale) {
	m_scale = scale;
	m_rate = rate;
	m_loadfile = true;
	m_numChapter = 0;
	ShowList();
}

void CfgDlg::ShowList(int nSelect) {
	char str[STRLEN+50];

	SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_RESETCONTENT, 0L, 0L);

	for(int n = 0;n < m_numChapter;n++) {
		std::string time_str = frame2time(m_Frame[n], m_rate, m_scale);
		sprintf_s(str,"%02d %06d [%s] %s\n",n + 1,m_Frame[n],time_str.c_str(),m_strTitle[n]);
		SendDlgItemMessage(m_hDlg,IDC_LIST1,LB_ADDSTRING,0L,(LPARAM)str);
	}

	if (nSelect != -1) {
		SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_SETTOPINDEX, (WPARAM)max(nSelect-3, 0), 0L);
		SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_SETCURSEL, (WPARAM)nSelect, 0L);
	}

	InvalidateRect(GetDlgItem(m_hDlg, IDC_TIMELINE), NULL, TRUE);
}

void CfgDlg::AddHis() {
	char str[STRLEN];

	GetDlgItemText(m_hDlg,IDC_EDNAME,str,STRLEN);

	//履歴に同じ文字列があれば履歴から削除
	for(int n = 0;n < m_numHis;n++) {
		if(strcmp(str,m_strHis[n]) == 0) {
			for(int i = n;i < m_numHis - 1;i++) memcpy(m_strHis[i],m_strHis[i+1],STRLEN);
			m_strHis[m_numHis - 1][0] = 0;
			m_numHis--;
		}
	}

	//履歴に追加
	if(str[0] != 0) {
		for(int n = NUMHIS - 1 ;n > 0;n--) memcpy(m_strHis[n],m_strHis[n-1],STRLEN);
		strcpy_s(m_strHis[0], str);
		m_numHis++;
	}
	if(m_numHis > NUMHIS) m_numHis = NUMHIS;

	//コンボボックスの表示更新
	SendDlgItemMessage(m_hDlg,IDC_EDNAME,CB_RESETCONTENT,0,0);
	for(int n = 0;n < m_numHis;n++) {
		SendDlgItemMessage(m_hDlg,IDC_EDNAME,CB_ADDSTRING,0L,(LPARAM)m_strHis[n]);
	}
	//[xt]エディットの文字列を維持する
	SetDlgItemText(m_hDlg,IDC_EDNAME,str);

	//iniに履歴を保存
	for(int n = 0;n < NUMHIS;n++) {
		sprintf_s(str, "history%d", n);
		m_exfunc->ini_save_str(m_fp, str, m_strHis[n]);
	}
}

void CfgDlg::AddList() {
	char str[STRLEN];
	int ins;

	if(m_loadfile == false) return;	//ファイルが読み込まれていない

	CreateUndoPoint();

	GetDlgItemText(m_hDlg,IDC_EDNAME,str,STRLEN);
	//if(str[0] == '\0') return;	//タイトルが入力されてなくてもおｋ(r13)

	for(ins = 0;ins < m_numChapter;ins++) {
		if(m_Frame[ins] == m_frame) {
			//タイムコードが重複しているときは、タイトルを変更(r13)
			strcpy_s(m_strTitle[ins], STRLEN, str);
			ShowList(ins);
			AddHis();
			return;
		}
		if(m_Frame[ins] > m_frame) break;
	}
	ReserveChapterList(m_numChapter+1);
	for(int n = m_numChapter;n > ins;n--) {
		m_Frame[n] = m_Frame[n-1];
		strcpy_s(m_strTitle[n],STRLEN,m_strTitle[n-1]);
	}
	m_numChapter++;

	strcpy_s(m_strTitle[ins],STRLEN,str);
	m_Frame[ins] = m_frame;
	ShowList(ins);
	AddHis();
}

void CfgDlg::DelList() {
	LRESULT sel;

	sel = SendDlgItemMessage(m_hDlg,IDC_LIST1,LB_GETCURSEL,0,0);
	if(sel == LB_ERR) return;

	if(m_loadfile == false) return;	//ファイルが読み込まれていない
	if(m_numChapter <= sel) return; //アイテムがない

	CreateUndoPoint();

	m_numChapter--;
	for(int n = sel;n < m_numChapter;n++) {
		m_Frame[n] = m_Frame[n+1];
		strcpy_s(m_strTitle[n],STRLEN,m_strTitle[n+1]);
	}
	ShowList(min(sel, m_numChapter - 1));
}

void CfgDlg::NextList() {
	if(m_loadfile == false){
		return;	//ファイルが読み込まれていない
	}
	LRESULT sel = SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_GETCURSEL, 0, 0);
	if(sel == LB_ERR) {
		return;
	}
	if (sel + 1 < m_numChapter) {
		SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_SETCURSEL, sel+1, 0);
		Seek();
	}
}

void CfgDlg::PrevList() {
	if(m_loadfile == false){
		return;	//ファイルが読み込まれていない
	}
	LRESULT sel = SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_GETCURSEL, 0, 0);
	if(sel == LB_ERR) {
		return;
	}
	if (0 < sel) {
		SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_SETCURSEL, sel-1, 0);
		Seek();
	}
}

void CfgDlg::NextHereList() {
	if(!m_loadfile || !m_numChapter){
		return;	//ファイルが読み込まれていない or チャプターがない
	}
	int frame = m_exfunc->get_frame(m_editp);
	for(int i=0; i<m_numChapter; ++i) {
		if (frame < m_Frame[i]) {
			SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_SETCURSEL, i, 0);
			Seek();
			return;
		}
	}
	// 最後にシーク
	SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_SETCURSEL, -1, 0);
	m_exfunc->set_frame(m_editp, m_exfunc->get_frame_n(m_editp) - 1);
}

void CfgDlg::PrevHereList() {
	if(!m_loadfile || !m_numChapter){
		return;	//ファイルが読み込まれていない or チャプターがない
	}
	int frame = m_exfunc->get_frame(m_editp);
	for(int i=0; i<m_numChapter; ++i) {
		int seekPos = m_Frame[i];
		{
			// [xt]通常チャプターのシークがおかしくなるので構文チェック
			char *endp;
			int n = strtol(m_strTitle[i], &endp, 10);
			if (m_strTitle[i] != endp && !strncmp(endp, "フレーム", sizeof("フレーム") - 1)) {
				seekPos += n + 5;
			}
		}
		if (frame <= seekPos) {
			if (i == 0) {
				// 最初にシーク
				SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_SETCURSEL, -1, 0);
				m_exfunc->set_frame(m_editp, 0);
				return;
			}
			SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_SETCURSEL, i-1, 0);
			Seek();
			return;
		}
	}
	// 最後のチャプターにシーク
	SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_SETCURSEL, m_numChapter - 1, 0);
	Seek();
}

void yuy2_to_luma( const unsigned char* __restrict yup, unsigned char* __restrict luma, int w, int max_w, int h )
{
	int scale = w * 2 <= max_w ? 2 : 1;
	int step = scale * 2;
	int j = 0;
	for(int y=0;y<h;y++){
		int i = max_w * 2 * (y * scale);
		for(int x=0;x<w;x++) {
			luma[j] = yup[i];
			i += step;
			j++;
		}
	}
}

//[ru]IIR_3DNRより拝借
#if 0
//---------------------------------------------------------------------
//		輝度を8ビットにシフトする関数
//---------------------------------------------------------------------
void shift_to_eight_bit( PIXEL_YC* ycp, unsigned char* luma, int w, int max_w, int h )
{
	int skip = max_w-w;
	for(int y=0;y<h;y++){
		for(int x=0;x<w;x++) {
			if(ycp->y & 0xf000)
				*luma = (ycp->y < 0) ? 0 : 255;
			else
				*luma = ycp->y >>4 ;
			ycp++;
			luma++;
		}
		ycp += skip;
	}
}
void shift_to_eight_bit_sse( PIXEL_YC* ycp, unsigned char* luma, int w, int max_w, int h )
{
	int skip = max_w-w;
	__m128i m4095 = _mm_set1_epi16(4095);
	__m128i m0 = _mm_setzero_si128();
	for(int jy=0;jy<h;jy++){
		for(int jx=0;jx<w/16;jx++) {
			//for (int j=0; j<8; j++)
			//	y.m128i_i16[j] = (ycp+j)->y;
			//__m128i y = _mm_setzero_si128();
			//y = _mm_insert_epi16(y, (ycp+0)->y, 0);
			__m128i y = _mm_load_si128((__m128i*)ycp);
			y = _mm_insert_epi16(y, (ycp+1)->y, 1);
			y = _mm_insert_epi16(y, (ycp+2)->y, 2);
			y = _mm_insert_epi16(y, (ycp+3)->y, 3);
			y = _mm_insert_epi16(y, (ycp+4)->y, 4);
			y = _mm_insert_epi16(y, (ycp+5)->y, 5);
			y = _mm_insert_epi16(y, (ycp+6)->y, 6);
			y = _mm_insert_epi16(y, (ycp+7)->y, 7);
			ycp += 8;
			
			//__m128i y1 = _mm_setzero_si128();
			//y1 = _mm_insert_epi16(y1, (ycp+0)->y, 0);
			__m128i y1 = _mm_load_si128((__m128i*)ycp);
			y1 = _mm_insert_epi16(y1, (ycp+1)->y, 1);
			y1 = _mm_insert_epi16(y1, (ycp+2)->y, 2);
			y1 = _mm_insert_epi16(y1, (ycp+3)->y, 3);
			y1 = _mm_insert_epi16(y1, (ycp+4)->y, 4);
			y1 = _mm_insert_epi16(y1, (ycp+5)->y, 5);
			y1 = _mm_insert_epi16(y1, (ycp+6)->y, 6);
			y1 = _mm_insert_epi16(y1, (ycp+7)->y, 7);

			//__m128i cmp =_mm_cmpgt_epi16(y, m4095);
			//y = _mm_or_si128(y, cmp);

			__m128i cmp = _mm_cmpgt_epi16(y, m0);
			y = _mm_and_si128(y, cmp);
			y = _mm_srli_epi16(y, 4);

			cmp = _mm_cmpgt_epi16(y1, m0);
			y1 = _mm_and_si128(y1, cmp);
			y1 = _mm_srli_epi16(y1, 4);

			//for (int j=0; j<8; j++)
			//	luma[j] = y.m128i_i8[j*2];

			_mm_stream_si128((__m128i*)luma, _mm_packus_epi16(y, y1));

			//if(ycp->y & 0xf000)
			//	*luma = (ycp->y < 0) ? 0 : 255;
			//else
			//	*luma = ycp->y >>4 ;
			ycp += 8;
			luma += 16;
		}
		ycp += skip;
	}
}
#endif
#define FRAME_PICTURE	1
#define FIELD_PICTURE	2
int mvec(unsigned char* current_pix,unsigned char* bef_pix,int lx,int ly,int threshold,int pict_struct);
//ここまで

//[ru] ジャンプウィンドウ更新
BOOL CALLBACK searchJump(HWND hWnd, LPARAM lParam) {
	TCHAR buf[1024];
	TCHAR frames[2][100];
	sprintf_s(frames[0], "/ %d ]", lParam);
	sprintf_s(frames[1], "/ %d ]", lParam-1);
	if (GetWindowText(hWnd, buf, 1024)) {
		// まずジャンプウィンドウを探す
		if (strncmp(buf, "ジャンプウィンドウ", 18) == 0) {
			// 次に総フレーム数が一致しているのを探す
			if (strstr(buf, frames[0]) || strstr(buf, frames[1])) {
				// みっけた
				if (IsWindowVisible(hWnd))
					PostMessage(hWnd, WM_COMMAND, 0x9c6b, 0); // lParamはなんだっけ・・・
				return FALSE;
			}
		}
	}
	return TRUE;
}
//ここまで

class CThreadProc
{
protected:
	HANDLE hThread;
	HANDLE hNotify[2];
	volatile bool bTerminate;

	//処理用パラメータ
	unsigned char* volatile pix1;
	unsigned char* volatile pix0;
	int w, h;
	int movtion_vector;
public:
	CThreadProc(){
		hThread = NULL;
		hNotify[0] = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		hNotify[1] = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		bTerminate = false;
	}
	~CThreadProc(){
		::CloseHandle(hThread);
		::CloseHandle(hNotify[0]);
		::CloseHandle(hNotify[1]);
	}
	void SetParam(unsigned char* p0_, unsigned char* p1_, int w_, int h_){
		pix0 = p0_;
		pix1 = p1_;
		w = w_;
		h = h_;
	}
	unsigned char* SetInputImage(unsigned char* pImage){
		unsigned char *tmp = pix0;
		pix0 = pix1;
		pix1 = pImage;
		return tmp;
	}

	void Run(){
		movtion_vector = mvec( pix1, pix0, w, h, (100-0)*(100/FIELD_PICTURE), FIELD_PICTURE);
	}
	static DWORD WINAPI ThreadProc(void* pParam){
		CThreadProc* pThis = (CThreadProc*)pParam;
		::WaitForSingleObject(pThis->hNotify[1], INFINITE);
		while(!pThis->bTerminate){
			pThis->Run();
			::SetEvent(pThis->hNotify[0]);
			::WaitForSingleObject(pThis->hNotify[1], INFINITE);
		}
		return 0;
	}
	void StartThread(){
		DWORD threadID;
		hThread = ::CreateThread(NULL, 0, ThreadProc, (void*)this, 0, &threadID);
	}
	void ResumeThread(){
		::SetEvent(hNotify[1]);
	}
	void WaitThread(){
		::WaitForSingleObject(hNotify[0], INFINITE);
	}
	void Terminate(){
		bTerminate = true;
		ResumeThread();
		::WaitForSingleObject(hThread, INFINITE);
	}
	int GetMovtionVector(){
		return movtion_vector;
	}
};

int CfgDlg::GetSCPos(int moveto, int frames)
{
	static const DWORD FORMAT_YUY2 = 'Y'|('U'<<8)|('Y'<<16)|((DWORD)'2'<<24);
	int srcw;
	int srch;
	if(!m_editp || !m_exfunc->get_frame_size(m_editp, &srcw, &srch)){
		return 0;
	}

	// [xt]HDサイズ以上の画像は縦横1/2に縮小(ボトルネックである動き検索を高速化するため)
	int scale = srcw * srch >= 1280 * 720 ? 2 : 1;
	int stride = (srcw + 1) / 2 * 2 * 2;
	int w = (srcw / scale) & 0xFFF0;
	int h = (srch / scale) & 0xFFF0;

	int max_motion = -1;
	int max_motion_frame = 0;

	// [xt]16bitDIB形式YUY2画像を格納
	void* yu = _aligned_malloc(stride * srch, 16);

	// 動きベクトルが最大値のフレームを検出
	unsigned char* pix2 = (unsigned char*)_aligned_malloc(w*h, 32);	//8ビットにシフトした現フレームの輝度が代入される
	unsigned char* pix1 = (unsigned char*)_aligned_malloc(w*h, 32);	//8ビットにシフトした現フレームの輝度が代入される
	unsigned char* pix0 = (unsigned char*)_aligned_malloc(w*h, 32);	//8ビットにシフトした前フレームの輝度が代入される
	unsigned char* pixFree = pix2;

	//計測タイマ
	QPC totalQPC;

	totalQPC.start();

	// [xt]get_ycp_source_cache()だとストライド不明で画像がずれるので、get_pixel_source()に置きかえてみる
	m_exfunc->get_pixel_source(m_editp, max(moveto-1, 0), yu, FORMAT_YUY2);
	yuy2_to_luma((unsigned char*)yu, pix0, w, stride/2, h);
	m_exfunc->get_pixel_source(m_editp, moveto, yu, FORMAT_YUY2);
	yuy2_to_luma((unsigned char*)yu, pix1, w, stride/2, h);

	CThreadProc thread;
	thread.SetParam(pix0, pix1, w, h);
	thread.StartThread();

	int checkFrame = min(frames+5,200); 
	for (int i=0; i<checkFrame; i++) {
		thread.ResumeThread();
		if((i+1) <checkFrame){
			m_exfunc->get_pixel_source(m_editp, moveto+i+1, yu, FORMAT_YUY2);
			yuy2_to_luma((unsigned char*)yu, pixFree, w, stride/2, h);
		}
		thread.WaitThread();
		pixFree = thread.SetInputImage(pixFree);

		int movtion_vector = thread.GetMovtionVector();
		if (movtion_vector > max_motion) {
			max_motion = movtion_vector;
			max_motion_frame = i;
		}
	}
	thread.Terminate();
	_aligned_free(yu);
	_aligned_free(pix2);
	_aligned_free(pix1);
	_aligned_free(pix0);

	totalQPC.stop();
#ifdef CHECKSPEED
	sprintf_s(str, "total: %.03f", totalQPC.get());
	MessageBox(NULL, str, NULL, 0);
	//OutputDebugString(str);
#endif

	return max_motion_frame;
}

void CfgDlg::Seek() {
	LRESULT sel;
	sel = SendDlgItemMessage(m_hDlg,IDC_LIST1,LB_GETCURSEL,0,0);
	if(sel == LB_ERR) return;
	if(m_Frame[sel] == m_frame) return;

	//[ru] シーンチェンジ検出
	//[xt] 通常チャプターのシークがおかしくなるので構文チェック
	char *endp;
	int frames = strtol(m_strTitle[sel], &endp, 10);
	if (m_strTitle[sel] == endp || strncmp(endp, "フレーム", sizeof("フレーム") - 1)) {
		frames = 0;
	}
	int moveto = m_Frame[sel];

	if (IsDlgButtonChecked(m_fp->hwnd, IDC_CHECKSC) && frames > 0) {	
		int max_motion_frame;
		const char *pscpos = strstr(m_strTitle[sel], "SCPos:");
		if (pscpos) {
			int scFrame = atoi(pscpos + sizeof("SCPos:") - 1);
			max_motion_frame = max(scFrame, 0);
		} else {
			max_motion_frame = GetSCPos(moveto, frames);
		}
		moveto += max_motion_frame;
	}
	m_exfunc->set_frame(m_editp, moveto);
	SetDlgItemText(m_hDlg,IDC_EDNAME,m_strTitle[sel]);
	EnumWindows(searchJump, (LPARAM)m_exfunc->get_frame_n(m_editp));
}

void CfgDlg::SetFrameN(void *editp,int frame_n) {
	m_numFrame = frame_n;
	m_editp = editp;
	if (frame_n == 0) {
		m_loadfile = false;
	}
	std::string time_str = frame2time(frame_n, m_rate, m_scale);
	char strTime[64];
	sprintf_s(strTime, "総時間 %s ", time_str.c_str());
	SetDlgItemText(m_hDlg, IDC_STATICTIME, strTime);
}

void CfgDlg::SetFrame(int frame) {
	std::string time_str = frame2time(frame, m_rate, m_scale);
	sprintf_s(m_strTime, "%s / %06d frame", time_str.c_str(), frame);
	SetDlgItemText(m_hDlg, IDC_EDTIME, m_strTime);
	m_frame = frame;
}

void CfgDlg::Save() {
	if(!m_numChapter) {
		return;
	}

	char path[_MAX_PATH];
	OPENFILENAME of;
	ZeroMemory(&of, sizeof(OPENFILENAME));
	ZeroMemory(path, sizeof(path));
		
	of.lStructSize = sizeof(OPENFILENAME);
	of.hwndOwner = m_hDlg;
	of.lpstrFile = path;
	of.nMaxFile = sizeof(path);

	char ext[STRLEN];
	if (!GetDlgItemText(m_hDlg, IDC_EDFILEEXT, ext, STRLEN) || ext[0] != '.') {
		strcpy_s(ext, ".txt");
	}
	FILE_INFO fip;
	if (m_editp && m_exfunc->get_file_info(m_editp, &fip) && (fip.flag & FILE_INFO_FLAG_VIDEO) && fip.name) {
		if (IsDlgButtonChecked(m_hDlg, IDC_CHECKDEFSAVE) == BST_CHECKED) {
			//[xt]既定保存
			strcpy_s(path, fip.name);
			PathRenameExtension(path, ext);
			//[xt]安全のため編集ファイル名と同じではないことを確認
			if (_stricmp(path, fip.name)) {
				SaveToFile(path);
			}
			return;
		}
		// [xt]編集ファイル名にチャプター拡張子をつけたものを初期保存名にする(これは嗜好が分かれるかもしれない)
		strcpy_s(path, PathFindFileName(fip.name));
		PathRenameExtension(path, ext);
	}
	char filter[STRLEN * 2 + 64];
	int len = sprintf_s(filter, "Chapter (*%s)", ext) + 1;
	len += sprintf_s(filter+len, sizeof(filter)-len, "*%s", ext) + 1;
	len += sprintf_s(filter+len, sizeof(filter)-len, "AllFile (*.*)") + 1;
	len += sprintf_s(filter+len, sizeof(filter)-len, "*.*") + 1;
	filter[len] = '\0';

	of.lpstrFilter = filter;
	of.nFilterIndex = 0;
	of.lpstrFileTitle = NULL;
	of.nMaxFileTitle = 0;
	of.lpstrInitialDir = NULL;
	of.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	if (GetSaveFileName(&of) == 0) {
		return;
	} 

	if (*PathFindExtension(path) == '\0') {
		strcat_s(path, ext);
	}

	if (PathFileExists(path)) {
		char message[_MAX_PATH + 128];
		sprintf_s(message, "%s は既に存在します。\nファイルを上書きしますか？", PathFindFileName(path));
		BOOL ret = MessageBox(m_hDlg, message, "チャプター編集", MB_YESNO|MB_DEFBUTTON2|MB_ICONWARNING);
		if(ret != IDYES) {
			return;
		}
	}

	SaveToFile(path);
}

void CfgDlg::AutoSave() {
	if (!m_numChapter || !m_autosave) {
		return;
	}

	char path[_MAX_PATH];
	my_getexepath(path, sizeof(path));
	strcat_s(path, sizeof(path), "chapter.txt");

	SaveToFile(path);
}

bool CfgDlg::SaveToFile(const char *lpFile) {
	char str[STRLEN+100];
	FILE *file;
	if (fopen_s(&file, lpFile, "w")) {
		MessageBox(m_hDlg, "出力ファイルを開けませんでした。", "チャプター編集", MB_OK|MB_ICONINFORMATION);
		return false;
	}
	ClearUndoPoint();

	for(int n = 0;n < m_numChapter;n++) {
		std::string time_str = frame2time(m_Frame[n], m_rate, m_scale);
		sprintf_s(str, "CHAPTER%02d=%s\n", n + 1, time_str.c_str());
		fputs(str, file);
		sprintf_s(str, "CHAPTER%02dNAME=%s\n", n + 1, m_strTitle[n]);
		fputs(str, file);
	}
	fclose(file);
	return true;
}

void CfgDlg::Load() {
	char path[_MAX_PATH];
	OPENFILENAME of;

	if(m_loadfile == false) return;

	ZeroMemory(&of,sizeof(OPENFILENAME));
	ZeroMemory(path,sizeof(path));
		
	of.lStructSize = sizeof(OPENFILENAME);
	of.hwndOwner = m_hDlg;
	of.lpstrFile = path;
	of.nMaxFile = sizeof(path);

	char ext[STRLEN];
	if (!GetDlgItemText(m_hDlg, IDC_EDFILEEXT, ext, STRLEN) || ext[0] != '.') {
		strcpy_s(ext, ".txt");
	}
	char filter[STRLEN * 2 + 64];
	int len = sprintf_s(filter, "Chapter (*%s)", ext) + 1;
	len += sprintf_s(filter+len, sizeof(filter)-len, "*%s", ext) + 1;
	len += sprintf_s(filter+len, sizeof(filter)-len, "AllFile (*.*)") + 1;
	len += sprintf_s(filter+len, sizeof(filter)-len, "*.*") + 1;
	filter[len] = '\0';

	of.lpstrFilter = filter;
	of.nFilterIndex = 0;
	of.lpstrFileTitle = NULL;
	of.nMaxFileTitle = 0;
	of.lpstrInitialDir = NULL;
	of.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	if(GetOpenFileName(&of) == 0) {
		return;
	}
	LoadFromFile(path);
}

void CfgDlg::LoadFromFile(char *filename) {
	FILE *file;
	char str[STRLEN+2];
	int h,m,s,ms;
	int frame;
	if(fopen_s(&file,filename,"r")) {
		MessageBox(NULL,"ファイルを開けませんでした。","チャプター編集",MB_OK|MB_ICONINFORMATION);
		return;
	}
	ClearUndoPoint();

	const std::tr1::basic_regex<TCHAR> re1("^CHAPTER(\\d\\d\\d?\\d?)=(\\d\\d):(\\d\\d):(\\d\\d)\\.(\\d\\d\\d)");
	const std::tr1::basic_regex<TCHAR> re2("^CHAPTER(\\d\\d\\d?\\d?)NAME=(.*)$");

	m_numChapter = 0;

	while(true) {
		// 時間の処理
		if(fgets(str,STRLEN,file) == NULL) break;
		//                       0123456789012345678901
		if(strlen(str) < sizeof("CHAPTER00=00:00:00.000") - 1) break;

		std::tr1::match_results<std::string::const_iterator> results;
		std::string stds(str);
		if (std::tr1::regex_search(stds, results, re1) == FALSE) {
			break;
		}
		h = atoi(results.str(2).c_str());
		m = atoi(results.str(3).c_str());
		s = atoi(results.str(4).c_str());
		ms = atoi(results.str(5).c_str());
		frame = time2frame(h, m, s, ms, m_rate, m_scale);

		// 名前の処理
		if(fgets(str,STRLEN,file) == NULL) break;
		//                       01234567890123
		if(strlen(str) < sizeof("CHAPTER00NAME=") - 1) break;

		// strip
		for(int i = 0;i < STRLEN;i++) {
			if(str[i] == '\n' || str[i] == '\r') {
				str[i] = '\0'; break;
			}
		}

		stds = str;
		if (std::tr1::regex_search(stds, results, re2) == FALSE) {
			break;
		}

		ReserveChapterList(m_numChapter + 1);
		m_Frame[m_numChapter] = frame;
		strcpy_s(m_strTitle[m_numChapter], results.str(2).c_str());

		// SC位置情報の取得
		const char *szSCPosMark = "SCPos:";
		char *pscpos = strstr(m_strTitle[m_numChapter], szSCPosMark);
		if (pscpos) {
			pscpos += strlen(szSCPosMark);
			int scFrame = atoi(pscpos);
			// [xt]仕様が絶対系か相対系か分からない(実際r19ではSC保存したチャプターを読み込ませるとシークがおかしくなる)が
			// 後ろのチャプターほど増えるというのもあまり綺麗じゃないので相対系とみなして修正
			// (chapter_exe.exeは絶対系なので相互運用は微妙になる)
			scFrame = max(scFrame, 0);
			
			// マーク付与
			if (IsDlgButtonChecked(m_fp->hwnd, IDC_SCMARK)){
				FRAME_STATUS frameStatus;
				m_exfunc->get_frame_status(m_editp, frame + scFrame, &frameStatus);
				frameStatus.edit_flag |= EDIT_FRAME_EDIT_FLAG_MARKFRAME;
				m_exfunc->set_frame_status(m_editp, frame + scFrame, &frameStatus);
			}
		}
		
		m_numChapter++;
	}
	fclose(file);
	ShowList();
}

// FAWチェックと、FAWPreview.aufを使っての1フレームデコード
class CFAW {
	bool is_half;

	bool load_failed;
	HMODULE _h;

	typedef int (__stdcall *ExtractDecode1FAW)(const short *in, int samples, short *out, bool is_half);
	ExtractDecode1FAW _ExtractDecode1FAW;

	bool load() {
		if (_ExtractDecode1FAW == NULL && load_failed == false) {
			_h = LoadLibrary("FAWPreview.auf");
			if (_h == NULL) {
				load_failed = true;
				return false;
			}
			_ExtractDecode1FAW = (ExtractDecode1FAW)GetProcAddress(_h, "ExtractDecode1FAW");
			if (_ExtractDecode1FAW == NULL) {
				FreeLibrary(_h);
				_h = NULL;
				load_failed = true;
				return false;
			}
			return true;
		}
		return true;
	}
public:
	CFAW() : _h(NULL), _ExtractDecode1FAW(NULL), load_failed(false), is_half(false) { }
	
	~CFAW() {
		if (_h) {
			FreeLibrary(_h);
		}
	}

	bool isLoadFailed(void) {
		return load_failed;
	}

	// FAW開始地点を探す。1/2なFAWが見つかれば、以降はそれしか探さない。
	// in: get_audio()で得た音声データ
	// samples: get_audio() * ch数
	// 戻り値：FAW開始位置のインデックス。なければ-1
	int findFAW(short *in, int samples) {
		// search for 72 F8 1F 4E 07 01 00 00
		static unsigned char faw11[] = {0x72, 0xF8, 0x1F, 0x4E, 0x07, 0x01, 0x00, 0x00};
		if (is_half == false) {
			for (int j=0; j<samples - 30; ++j) {
				if (memcmp(in+j, faw11, sizeof(faw11)) == 0) {
					return j;
				}
			}
		}

		// search for 00 F2 00 78 00 9F 00 CE 00 87 00 81 00 80 00 80
		static unsigned char faw12[] = {0x00, 0xF2, 0x00, 0x78, 0x00, 0x9F, 0x00, 0xCE,
										0x00, 0x87, 0x00, 0x81, 0x00, 0x80, 0x00, 0x80};

		for (int j=0; j<samples - 30; ++j) {
			if (memcmp(in+j, faw12, sizeof(faw12)) == 0) {
				is_half = true;
				return j;
			}
		}

		return -1;
	}

	// FAWPreview.aufを使ってFAWデータ1つを抽出＆デコードする
	// in: FAW開始位置のポインタ。findFAWに渡したin + findFAWの戻り値
	// samples: inにあるデータのshort換算でのサイズ
	// out: デコード結果を入れるバッファ(16bit, 2chで1024サンプル)
	//     （1024sample * 2byte * 2ch = 4096バイト必要）
	int decodeFAW(const short *in, int samples, short *out){
		if (load()) {
			return _ExtractDecode1FAW(in, samples, out, is_half);
		}
		return 0;
	}
};

class CMute {
private:
	short _buf[48000];

public:
	CMute() {}
	~CMute() {}

	static bool isMute(short *buf, int naudio, short mute) {
		for (int j=0; j<naudio; ++j) {
			if (abs(buf[j]) > mute) {
				return false;
			}
		}
		return true;
	}
};

static BOOL CALLBACK EnableChildWindowProc(HWND hwnd, LPARAM lParam) {
	EnableWindow(hwnd, (BOOL)lParam);
	return TRUE;
}

//[ru]無音部分検出
void CfgDlg::DetectMute() {
	if(m_loadfile == false)
		return;	//ファイルが読み込まれていない

	FILE_INFO fip;
	if (!m_exfunc->get_file_info(m_editp, &fip))
		return; // 情報を取得できない

	if ((fip.flag & FILE_INFO_FLAG_AUDIO) == 0) {
		MessageBox(m_hDlg, "音声トラックがありません", NULL, MB_OK);
		return;
	}

	int seri = GetDlgItemInt(m_hDlg, IDC_EDITSERI, NULL, FALSE);
	int mute = GetDlgItemInt(m_hDlg, IDC_EDITMUTE, NULL, FALSE);

	if (seri < 5) {
		MessageBox(m_hDlg, "無音連続フレーム数は 5 以上を指定してください", NULL, MB_OK);
		return;
	}
	if (mute < 0 || 1 << 15 < mute) {
		MessageBox(m_hDlg, "無音閾値は 0 ～ 2^15 の範囲で指定してください", NULL, MB_OK);
		return;
	}
	
	m_exfunc->ini_save_int(m_fp, "muteCount", seri);
	m_exfunc->ini_save_int(m_fp, "muteLimit", mute);

	int n = m_exfunc->get_frame_n(m_editp);

	CreateUndoPoint();

	// チャプター個数
	// [xt]作業中にメッセージを回すため、チャプター数と配列内容に不整合があるとまずいのでエイリアスにした
	m_numChapter = 0;
	int &pos = m_numChapter;

	// 適当にでかめにメモリ確保
	short buf[48000*2];
	FRAME_STATUS fs;
	int bvid = -10;
	if (m_exfunc->get_frame_status(m_editp, 0, &fs))
		bvid = fs.video;

	int start_fr = 0;	// 無音の開始フレーム
	int mute_fr = 0;	// 無音フレーム数
	bool isFAW = false;	// FAW使用かどうか（最初の200フレームで検出）
	CFAW cfaw;

	// FAWチェック
	int fawCount = 0;
	for (int i=0; i<min(200, n); ++i) {
		int naudio = m_exfunc->get_audio_filtered(m_editp, i, buf);
		if (naudio) {
			naudio *= fip.audio_ch;
			int j = cfaw.findFAW(buf, naudio);
			if (j != -1) {
				fawCount++;
			}
		}
	}
	if (fawCount > 10) {
		isFAW = true;
	}

	int progressCount = -1;
	char origCaption[64];
	if (!GetWindowText(m_hDlg, origCaption, sizeof(origCaption))) {
		origCaption[0] = '\0';
	}
	// [xt]コントロールと「閉じる」ボタンを無効化
	EnumChildWindows(m_hDlg, EnableChildWindowProc, FALSE);
	SetClassLong(m_hDlg, GCL_STYLE, GetClassLong(m_hDlg, GCL_STYLE) | CS_NOCLOSE);

	// フレームごとに音声を解析
	int skip = 0;
	for (int i=0; i<n; ++i) {
		// [xt]進捗をキャプションで表示
		if (progressCount != i * 100 / n) {
			progressCount = i * 100 / n;
			char caption[128];
			sprintf_s(caption, "%s (解析中%3d%%)", origCaption, progressCount);
			SetWindowText(m_hDlg, caption);
			// [xt]"(応答なし)"を防ぐためにダイアログ宛のメッセージだけ回しておく。メンバ変数の整合性に注意
			MSG msg;
			while (PeekMessage(&msg, m_hDlg, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		// 音声とフレームステータス取得
		if (!m_exfunc->get_frame_status(m_editp, i, &fs)) {
			continue;
		}

		// 編集点を検出
		int diff = fs.video - bvid;
		bvid = fs.video;
		if (diff && diff != 1) {
			ReserveChapterList(pos + 1);
			if (diff & 0xff000000)
				sprintf_s(m_strTitle[pos], STRLEN, "ソースチェンジ");
			else
				sprintf_s(m_strTitle[pos], STRLEN, "編集点 (間隔：%d)", diff);
			m_Frame[pos] = i;
			++pos;
			mute_fr = 0;
			start_fr = i;
			continue;
		}

		if (skip) {
			skip--;
			continue;
		}

		// 先フレームを読んで音があれば飛ばす
		if (i && mute_fr == 0 ) {
			int naudio = m_exfunc->get_audio_filtered(m_editp, i + seri - 1, buf);
			if (naudio && isFAW) {
				naudio *= fip.audio_ch;
				int j = cfaw.findFAW(buf, naudio);
				if (j != -1) {
					naudio = cfaw.decodeFAW(buf + j, naudio - j, buf);
				}
			}
			if (naudio) {
				if (!CMute::isMute(buf, naudio, mute)) {
					skip = seri - 1;
					continue;
				}
			}
		}
		
		int naudio = m_exfunc->get_audio_filtered(m_editp, i, buf);
		if (naudio == 0)
			continue;

		// ch数で調整
		naudio *= fip.audio_ch;

		// FAWをデコード
		if (isFAW) {
			int j = cfaw.findFAW(buf, naudio);
			if (j != -1) {
				naudio = cfaw.decodeFAW(buf+j, naudio-j, buf);

				if (cfaw.isLoadFailed()) {
					MessageBox(this->m_fp->hwnd, "FAWをデコードするのに 11/02/06以降のFAWPreview.auf（FAWぷれびゅ～） が必要です。", "エラー", MB_OK);
					SetWindowText(m_hDlg, origCaption);
					SetClassLong(m_hDlg, GCL_STYLE, GetClassLong(m_hDlg, GCL_STYLE) & ~CS_NOCLOSE);
					EnumChildWindows(m_hDlg, EnableChildWindowProc, TRUE);
					return ;
				}
			}
		}

		if (CMute::isMute(buf, naudio, mute)) {
			// 無音フレームだった
			if (mute_fr == 0) {
				start_fr = i;
			}
			++mute_fr;
		} else {
			// 無音じゃなかった
			// 基準フレーム数以上連続無音だったら
			if (mute_fr >= seri) {
				// 前回との差分が14～16秒だったら★をつける
				char *mark = "";
				if (pos > 0 && abs(start_fr - m_Frame[pos-1] - 30*15) < 30) {
					mark = "★";
				} else if (pos > 0 && abs(start_fr - m_Frame[pos-1] - 30*30) < 30) {
					mark = "★★";
				} else if (pos > 0 && abs(start_fr - m_Frame[pos-1] - 30*45) < 30) {
					mark = "★★★";
				} else if (pos > 0 && abs(start_fr - m_Frame[pos-1] - 30*60) < 30) {
					mark = "★★★★";
				}
				
				if (pos && (start_fr - m_Frame[pos-1] <= 1)) {
					sprintf_s(m_strTitle[pos-1], STRLEN, "%s 無音%02dフレーム %s", m_strTitle[pos-1], mute_fr, mark);
				} else {
					ReserveChapterList(pos + 1);
					sprintf_s(m_strTitle[pos], STRLEN, "%02dフレーム %s", mute_fr, mark);
					m_Frame[pos] = start_fr;

					if (IsDlgButtonChecked(m_fp->hwnd, IDC_PRECHECK)){
						int scFrame = GetSCPos(start_fr, mute_fr);
						sprintf_s(m_strTitle[pos], STRLEN, "%02dフレーム %s SCPos:%d", mute_fr, mark, scFrame);
						if (IsDlgButtonChecked(m_fp->hwnd, IDC_SCMARK)){
							int target_frame = start_fr + scFrame;
							FRAME_STATUS frameStatus;
							m_exfunc->get_frame_status(m_editp, target_frame, &frameStatus);
							frameStatus.edit_flag |= EDIT_FRAME_EDIT_FLAG_MARKFRAME;
							m_exfunc->set_frame_status(m_editp, target_frame, &frameStatus);
						}
					}

					++pos;
				}
			}
			mute_fr = start_fr = 0;
		}
	}
	
	SetWindowText(m_hDlg, origCaption);
	SetClassLong(m_hDlg, GCL_STYLE, GetClassLong(m_hDlg, GCL_STYLE) & ~CS_NOCLOSE);
	EnumChildWindows(m_hDlg, EnableChildWindowProc, TRUE);
	ShowList();
}
//ここまで

void CfgDlg::UpdateFramePos()
{
	int stFrame, edFrame;
	m_exfunc->get_select_frame(m_editp, &stFrame, &edFrame);
	int diff = edFrame - stFrame + 1;

	int nShowing, toSelect = -1;
	nShowing = m_exfunc->get_frame(m_editp);

	CreateUndoPoint();

	int orgNum = m_numChapter;
	m_numChapter = 0;
	int pos = 0; // 新しい位置
	int diffTotal = diff;
	for(int n=0; n<orgNum; n++){
		if(stFrame <= m_Frame[n] && m_Frame[n] <= edFrame){
			// [xt]間に編集点があれば加算する
			static const char szMark[] = "編集点 (間隔：";
			if(!strncmp(m_strTitle[n], szMark, sizeof(szMark) - 1)){
				int m = atoi(m_strTitle[n] + sizeof(szMark) - 1);
				diffTotal += max(m, 0);
			}
			continue;
		}
		m_Frame[pos] = m_Frame[n];
		if(m_Frame[n] > edFrame){
			m_Frame[pos] -= diff;
		}
		if(pos != n){
			strcpy_s(m_strTitle[pos], m_strTitle[n]);
		}
		pos++;
	}
	// [xt]間にチャプターがないと編集点が出力されないのを修正
	ReserveChapterList(++pos);
	for(int n=pos-1; n>=0; n--){
		if(n == 0 || m_Frame[n-1] < stFrame){
			m_Frame[n] = stFrame;
			sprintf_s(m_strTitle[n], "編集点 (間隔：%d)", diffTotal);
			break;
		}
		m_Frame[n] = m_Frame[n-1];
		strcpy_s(m_strTitle[n],m_strTitle[n-1]);
	}
	// [xt]現在表示フレームかその手前を選択する仕様(だと思う)がそうなってなかったのを修正
	if(nShowing > edFrame){
		nShowing -= diff;
	}else if(nShowing > stFrame){
		nShowing = stFrame;
	}
	for(int n=0; n<pos && nShowing>=m_Frame[n]; n++){
		// 選択位置の決定
		toSelect = n;
	}
	m_numChapter = pos;
	ShowList(toSelect);
	if(0 <= toSelect && toSelect < m_numChapter){
		SetDlgItemText(m_hDlg,IDC_EDNAME,m_strTitle[toSelect]);
	}
}

void CfgDlg::UpdateDlgItems()
{
	// 表示されているときは再描画
	if (m_exfunc->is_filter_window_disp(m_fp)) {
		UpdateWindow(GetDlgItem(m_hDlg, IDC_EDTIME));
		HWND hItem = GetDlgItem(m_hDlg, IDC_TIMELINE);
		InvalidateRect(hItem, NULL, TRUE);
		UpdateWindow(hItem);
		UpdateThumbs();
	}
}

void CfgDlg::DelMuteChapters()
{
	if (!m_loadfile) return;
	CreateUndoPoint();

	int n = 0;
	for (int i = 0; i < m_numChapter; i++) {
		char *endp;
		strtol(m_strTitle[i], &endp, 10);
		if (m_strTitle[i] == endp || strncmp(endp, "フレーム", sizeof("フレーム") - 1)) {
			if (i != n) {
				m_Frame[n] = m_Frame[i];
				strcpy_s(m_strTitle[n], m_strTitle[i]);
			}
			n++;
		}
	}
	if (m_numChapter != n) {
		m_numChapter = n;
		ShowList();
	}
}

void CfgDlg::ExpandEditPointChapters()
{
	if (!m_loadfile) return;
	CreateUndoPoint();

	// 開始と終了のチャプター名を置きかえる隠し設定
	char szStartMark[STRLEN];
	if (!m_exfunc->ini_load_str(m_fp, "editStartMark", szStartMark, "") || !szStartMark[0]) {
		strcpy_s(szStartMark, "編集点開始");
	}
	char szEndMark[STRLEN];
	if (!m_exfunc->ini_load_str(m_fp, "editEndMark", szEndMark, "") || !szEndMark[0]) {
		strcpy_s(szEndMark, "編集点終了");
	}

	// 編集点の間隔だけ後続のチャプター位置をずらす
	int n = 0;
	int diffTotal = 0;
	static const char szMark[] = "編集点 (間隔：";
	for (int i = 0; i < m_numChapter; i++) {
		m_Frame[i] += diffTotal;
		if (!strncmp(m_strTitle[i], szMark, sizeof(szMark) - 1)) {
			int m = atoi(m_strTitle[i] + sizeof(szMark) - 1);
			diffTotal += max(m, 1);
			n++;
		}
		n++;
	}

	// 編集点のチャプターを開始と終了に分割
	ReserveChapterList(n);
	for (int i = m_numChapter - 1, j = n - 1; i >= 0; i--, j--) {
		if (!strncmp(m_strTitle[i], szMark, sizeof(szMark) - 1)) {
			int m = atoi(m_strTitle[i] + sizeof(szMark) - 1);
			m_Frame[j] = m_Frame[i] + max(m - 1, 1);
			strcpy_s(m_strTitle[j], szEndMark);
			m_Frame[--j] = m_Frame[i];
			strcpy_s(m_strTitle[j], szStartMark);
		} else {
			if (i != j) {
				m_Frame[j] = m_Frame[i];
				strcpy_s(m_strTitle[j], m_strTitle[i]);
			}
		}
	}
	if (m_numChapter != n) {
		m_numChapter = n;
		ShowList();
	}
}

static bool InsertRadioCheckMenuItem(HMENU hmenu, UINT item, UINT id, LPSTR str, bool check)
{
    MENUITEMINFO mi;
    mi.cbSize = sizeof(MENUITEMINFO);
    mi.fMask = MIIM_ID | MIIM_STATE | MIIM_TYPE;
    mi.wID = id;
    mi.fState = check ? MFS_CHECKED : 0;
    mi.fType = MFT_STRING | MFT_RADIOCHECK;
    mi.dwTypeData = str;
    return InsertMenuItem(hmenu, item, TRUE, &mi) != FALSE;
}

// ポップアップメニュー選択でサムネイル表示設定
bool CfgDlg::SetupThumbs()
{
	int selID = 0;
	HMENU hmenu = CreatePopupMenu();
	if (hmenu) {
		HMENU hSubMenu = CreatePopupMenu();
		if (hSubMenu) {
			InsertRadioCheckMenuItem(hSubMenu, 0, 1, "4:3", m_thumbAspect == 120);
			InsertRadioCheckMenuItem(hSubMenu, 1, 2, "16:9", m_thumbAspect == 90);
			AppendMenu(hmenu, MF_POPUP, (UINT_PTR)hSubMenu, "アスペクト比");
		}
		hSubMenu = CreatePopupMenu();
		if (hSubMenu) {
			for (int i = 0; i + 3 <= NUMTHUMBS; i++) {
				char str[16];
				sprintf_s(str, "%d", i + 3);
				InsertRadioCheckMenuItem(hSubMenu, i, i + 13, str, m_thumbsNum == i + 3);
			}
			AppendMenu(hmenu, MF_POPUP, (UINT_PTR)hSubMenu, "フレーム数");
		}
		POINT pt = {0};
		GetCursorPos(&pt);
		selID = TrackPopupMenu(hmenu, TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, m_hDlg, NULL);

		if (selID == 1 || selID == 2) {
			m_thumbAspect = selID == 1 ? 120 : 90;
			m_exfunc->ini_save_int(m_fp, "thumbAspect", m_thumbAspect);
		} else if (1 <= selID - 10 && selID - 10 <= NUMTHUMBS) {
			m_thumbsNum = selID - 10;
			m_exfunc->ini_save_int(m_fp, "thumbsNum", m_thumbsNum);
		}
		DestroyMenu(hmenu);
	}
	return selID != 0;
}

void CfgDlg::ProjectSave(void *data, int *size) const
{
	if (!data) {
		*size = sizeof(PrfDat);
	} else if (*size == sizeof(PrfDat)) {
		// 互換のため未使用領域も保存するが.aupはどうやら連長圧縮されてるっぽいのでクリアしておく
		ZeroMemory(data, *size);
		PrfDat *prf = (PrfDat*)data;
		prf->m_numChapter = min(100, m_numChapter);
		CopyMemory(prf->m_Frame, m_Frame, sizeof(int) * prf->m_numChapter);
		for (int i = 0; i < prf->m_numChapter; i++) strcpy_s(prf->m_strTitle[i], m_strTitle[i]);
	}
}

void CfgDlg::ProjectLoad(const void *data, int size)
{
	if (size == sizeof(PrfDat)) {
		const PrfDat *prf = (const PrfDat*)data;
		m_numChapter = prf->m_numChapter;
		ReserveChapterList(m_numChapter);
		CopyMemory(m_Frame, prf->m_Frame, sizeof(int) * m_numChapter);
		CopyMemory(m_strTitle, prf->m_strTitle, sizeof(m_strTitle[0]) * m_numChapter);
		ShowList();
	}
}

void CfgDlg::CreateUndoPoint()
{
	SetDlgItemText(m_hDlg, IDC_BUSAVE, "保存*");
}

void CfgDlg::ClearUndoPoint()
{
	SetDlgItemText(m_hDlg, IDC_BUSAVE, "保存");
}

// チャプターリストを格納する領域を確保する
// チャプター数を増やす直前に必ず呼ぶ
void CfgDlg::ReserveChapterList(int numCapacity)
{
	if (m_numChapterCapacity < numCapacity) {
		m_numChapterCapacity = numCapacity + 16;
		m_Frame = (int*)realloc(m_Frame, sizeof(int) * m_numChapterCapacity);
		m_strTitle = (char(*)[STRLEN])realloc(m_strTitle, sizeof(m_strTitle[0]) * m_numChapterCapacity);
	}
	// どうせヌルポインタ例外になるので確保失敗チェックはしない
}

void CfgDlg::FreeChapterList()
{
	m_numChapterCapacity = 0;
	free(m_Frame);
	free(m_strTitle);
	m_Frame = NULL;
	m_strTitle = NULL;
}

// 24bitDIB形式画像を32bitビットマップにコピー
static void CopyToBitmap(HBITMAP hbmDest, const unsigned char* __restrict src, int srcw, int srch)
{
	DWORD *dest, destw, desth, field, offset, step, k;
	BITMAP bm;
	if (srcw<=0 || srch<=0 || !GetObject(hbmDest, sizeof(bm), &bm) || bm.bmWidth<=0 || bm.bmHeight<=0) return;

	dest = (DWORD*)bm.bmBits;
	destw = bm.bmWidth;
	desth = bm.bmHeight;
	for (DWORD j = 0; j < desth; j++) {
		// インタレースとの同期を防ぐため
		field = srch / desth <= 0 ? 0 : j % (srch / desth);
		offset = (srcw * 3 + 3) / 4 * 4 * (j * srch / desth + field);
		step = (srcw << 8) / destw;
		for (DWORD i = 0; i < destw; i++) {
			k = offset + ((i * step) >> 8) * 3;
			dest[j * destw + i] = src[k] | (src[k+1] << 8) | (src[k+2] << 16);
		}
	}
}

void CfgDlg::UpdateThumbs()
{
	int srcw;
	int srch;
	if (!m_editp || !m_exfunc->get_frame_size(m_editp, &srcw, &srch)) return;

	int stride = (srcw * 3 + 3) / 4 * 4;
	void *rgb = _aligned_malloc(stride * srch, 16);
	int frame = m_exfunc->get_frame(m_editp) - (m_thumbsNum - 1) / 2;
	int frameNum = m_exfunc->get_frame_n(m_editp);
	for (int i = 0; i < m_thumbsNum; i++, frame++) {
		// 必要ならサムネイル用ビットマップをリサイズ
		HWND hItem = GetDlgItem(m_hDlg, IDC_THUMBS_MIN + i);
		BITMAP bm;
		if (m_hbmThumbs[i] && (!GetObject(m_hbmThumbs[i], sizeof(bm), &bm) ||
		                       bm.bmWidth != m_thumbWidth || bm.bmHeight != m_thumbHeight))
		{
			HBITMAP hbmOld = (HBITMAP)SendMessage(hItem, STM_SETIMAGE, IMAGE_BITMAP, NULL);
			if (hbmOld && hbmOld != m_hbmThumbs[i]) {
				DeleteObject(hbmOld);
			}
			DeleteObject(m_hbmThumbs[i]);
			m_hbmThumbs[i] = NULL;
		}
		if (!m_hbmThumbs[i] && m_thumbWidth > 0 && m_thumbHeight > 0) {
			BITMAPINFO bmi = {0};
			bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth = m_thumbWidth;
			bmi.bmiHeader.biHeight = m_thumbHeight;
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 32;
			bmi.bmiHeader.biCompression = BI_RGB;
			void *pBits;
			m_hbmThumbs[i] = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
		}
		if (!m_hbmThumbs[i]) continue;

		// 現在のフレームを取得してコントロールにビットマップを登録
		HBITMAP hbmOld;
		if (0 <= frame && frame < frameNum) {
			m_exfunc->get_pixel_source(m_editp, frame, rgb, 0);
			CopyToBitmap(m_hbmThumbs[i], (unsigned char*)rgb, srcw, srch);
			hbmOld = (HBITMAP)SendMessage(hItem, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)m_hbmThumbs[i]);
		} else {
			hbmOld = (HBITMAP)SendMessage(hItem, STM_SETIMAGE, IMAGE_BITMAP, NULL);
		}
		if (hbmOld && hbmOld != m_hbmThumbs[i]) {
			DeleteObject(hbmOld);
		}
	}
	_aligned_free(rgb);
}

void CfgDlg::ClearThumbs()
{
	for (int i = 0; i < NUMTHUMBS; i++) {
		if (m_hbmThumbs[i]) {
			// コントロールからサムネイル用ビットマップを登録解除して解放
			HWND hItem = GetDlgItem(m_hDlg, IDC_THUMBS_MIN + i);
			HBITMAP hbmOld = (HBITMAP)SendMessage(hItem, STM_SETIMAGE, IMAGE_BITMAP, NULL);
			if (hbmOld && hbmOld != m_hbmThumbs[i]) {
				DeleteObject(hbmOld);
			}
			DeleteObject(m_hbmThumbs[i]);
			m_hbmThumbs[i] = NULL;
		}
	}
}

int CfgDlg::FindNearestChapter(int num, int denum, int threshold) const
{
	int chapter = -1;
	if (m_editp && denum > 0) {
		int frameNum = m_exfunc->get_frame_n(m_editp);
		int framePos = num * frameNum / denum;
		int minDistance = threshold * frameNum / denum;
		for (int i = 0; i < m_numChapter; i++) {
			int distance = abs(m_Frame[i] - framePos);
			if (distance < minDistance) {
				minDistance = distance;
				chapter = i;
			}
		}
	}
	return chapter;
}

static void DrawChapterTitle(HDC hdc, int x, int i, const char (*strTitle)[STRLEN], COLORREF crText, COLORREF crBk)
{
	COLORREF crOldTextColor = SetTextColor(hdc, crText);
	COLORREF crOldBkColor = SetBkColor(hdc, crBk);
	RECT rc = {x, 0, x + 256, 13};
	char str[STRLEN + 64];
	sprintf_s(str, "(%02d:%s)", i + 1, strTitle[i]);
	DrawText(hdc, str, -1, &rc, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
	SetBkColor(hdc, crOldBkColor);
	SetTextColor(hdc, crOldTextColor);
}

LRESULT CALLBACK CfgDlg::TimelineWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	CfgDlg *pThis = (CfgDlg*)GetWindowLong(hwnd, GWL_USERDATA);
	switch(message) {
	case WM_MOUSELEAVE:
		pThis->m_mouseTracking = false;
		pThis->m_hoveredChapter = -1;
		InvalidateRect(hwnd, NULL, TRUE);
		break;
	case WM_MOUSEMOVE:
		{
			RECT rc;
			GetClientRect(hwnd, &rc);
			int hovered = pThis->FindNearestChapter(LOWORD(lparam), rc.right - 80, 40);
			if (hovered != pThis->m_hoveredChapter) {
				pThis->m_hoveredChapter = hovered;
				InvalidateRect(hwnd, NULL, TRUE);
			}
			if (!pThis->m_mouseTracking) {
				TRACKMOUSEEVENT tme;
				tme.cbSize = sizeof(tme);
				tme.dwFlags = TME_LEAVE;
				tme.hwndTrack = hwnd;
				if (TrackMouseEvent(&tme)) {
					pThis->m_mouseTracking = true;
				}
			}
		}
		break;
	case WM_LBUTTONUP:
		{
			RECT rc;
			GetClientRect(hwnd, &rc);
			int hovered = pThis->FindNearestChapter(LOWORD(lparam), rc.right - 80, 40);
			if (hovered >= 0) {
				// 疑似的にリストボックスの選択動作をする
				HWND hItem = GetDlgItem(pThis->m_hDlg, IDC_LIST1);
				SendMessage(hItem, LB_SETCURSEL, (WPARAM)hovered, 0);
				SendMessage(pThis->m_hDlg, WM_COMMAND, MAKEWPARAM(IDC_LIST1, LBN_SELCHANGE), (LPARAM)hItem);
			}
		}
		break;
	case WM_PAINT:
		CallWindowProc(pThis->m_pStaticWndProc, hwnd, message, wparam, lparam);
		if (pThis->m_editp) {
			RECT rc;
			GetClientRect(hwnd, &rc);
			rc.right -= 80;
			int frameNum = pThis->m_exfunc->get_frame_n(pThis->m_editp);
			if (rc.right > 0 && rc.bottom > 0 && frameNum > 0) {
				int x;
				HDC hdc = GetDC(hwnd);
				HGDIOBJ hFontOld = SelectObject(hdc, (HGDIOBJ)SendMessage(hwnd, WM_GETFONT, 0, 0));
				int oldBkMode = SetBkMode(hdc, OPAQUE);

				// タイムライン背景
				HBRUSH hbrBlue = CreateSolidBrush(RGB(208,208,255));
				HGDIOBJ hbrOld = SelectObject(hdc, hbrBlue);
				HGDIOBJ hpenOld = SelectObject(hdc, GetStockObject(NULL_PEN));
				Rectangle(hdc, 0, 13, rc.right, 22);
				SelectObject(hdc, hpenOld);
				SelectObject(hdc, hbrOld);
				DeleteObject(hbrBlue);

				// チャプター位置の縦棒
				hpenOld = SelectObject(hdc, GetStockObject(BLACK_PEN));
				int lastDrawPos = INT_MIN;
				for (int i = 0; i < pThis->m_numChapter; i++) {
					x = pThis->m_Frame[i] * rc.right / frameNum;
					MoveToEx(hdc, x, 13, NULL);
					LineTo(hdc, x, 21);
					if (x - 48 >= lastDrawPos) {
						lastDrawPos = x;
						DrawChapterTitle(hdc, x, i, pThis->m_strTitle, RGB(0,0,0), GetSysColor(COLOR_3DFACE));
					}
				}
				SelectObject(hdc, hpenOld);

				hbrOld = SelectObject(hdc, GetStockObject(WHITE_BRUSH));
				//{
				// リストボックス選択中のチャプター
				HPEN hpenHL = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_HIGHLIGHT));
				hpenOld = SelectObject(hdc, hpenHL);
				int selChapter = SendDlgItemMessage(pThis->m_hDlg, IDC_LIST1, LB_GETCURSEL, 0, 0);
				if (0 <= selChapter && selChapter < pThis->m_numChapter) {
					x = pThis->m_Frame[selChapter] * rc.right / frameNum;
					Rectangle(hdc, x-1, 12, x+2, 22);
					DrawChapterTitle(hdc, x, selChapter, pThis->m_strTitle, GetSysColor(COLOR_HIGHLIGHTTEXT), GetSysColor(COLOR_HIGHLIGHT));
				}
				SelectObject(hdc, hpenOld);
				DeleteObject(hpenHL);

				// 現在編集中のフレーム位置
				HPEN hpenRed = CreatePen(PS_SOLID, 1, RGB(255,0,0));
				hpenOld = SelectObject(hdc, hpenRed);
				int frame = pThis->m_exfunc->get_frame(pThis->m_editp);
				x = frame * rc.right / frameNum;
				Rectangle(hdc, x-1, 12, x+1, 22);
				Rectangle(hdc, x, 12, x+2, 22);
				for (int i = 0; i < pThis->m_numChapter; i++) {
					if (pThis->m_Frame[i] == frame) {
						DrawChapterTitle(hdc, x, i, pThis->m_strTitle, RGB(255,255,255), RGB(255,0,0));
					}
				}
				SelectObject(hdc, hpenOld);
				DeleteObject(hpenRed);

				// ホバー中のチャプター
				hpenOld = SelectObject(hdc, GetStockObject(BLACK_PEN));
				if (0 <= pThis->m_hoveredChapter && pThis->m_hoveredChapter < pThis->m_numChapter) {
					x = pThis->m_Frame[pThis->m_hoveredChapter] * rc.right / frameNum;
					Rectangle(hdc, x-1, 12, x+2, 22);
					DrawChapterTitle(hdc, x, pThis->m_hoveredChapter, pThis->m_strTitle, RGB(255,255,255), RGB(0,0,0));
				}
				SelectObject(hdc, hpenOld);
				//}
				SelectObject(hdc, hbrOld);

				SetBkMode(hdc, oldBkMode);
				SelectObject(hdc, hFontOld);
				ReleaseDC(hwnd, hdc);
			}
		}
		return 0;
	}
	return CallWindowProc(pThis->m_pStaticWndProc, hwnd, message, wparam, lparam);
}
