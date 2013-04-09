//---------------------------------------------------------------------
//		プラグイン設定ヘッダファイル
//---------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <vector>
#include <string>
#include "filter.h"

#ifndef _CHAPTER_CONFIG_H_
#define _CHAPTER_CONFIG_H_

const int NUMHIS = 50;	// 保存する履歴の数
const int STRLEN = 256;	// 文字列の最大長
const int NUMTHUMBS = IDC_THUMBS_MAX - IDC_THUMBS_MIN + 1;

class CfgDlg;

// aupに保存する内容。現状100個まで
typedef struct 
{
	// チャプタ数は書式上最大100個まで
	int m_numChapter;
	int m_Frame[100];
	char m_strTitle[100][STRLEN];
	int m_SCPos[100];
} PrfDat;

// aupに保存する内容。チャプタ数制限なし
typedef struct {
	int frame;
	int reserved[3]; // 予約(0で初期化)
	char strTitle[STRLEN];
} ChapterElem;

typedef struct {
	PrfDat prf; // 後方互換+予約(0で初期化)
	int numChapter;
	ChapterElem elems[1];
} PrfDatEx;

//typedef struct {
//	int frame;
//	std::string title;
//} chapter;

class CfgDlg
{
	HWND m_hDlg;
	FILTER *m_fp;
	EXFUNC *m_exfunc;
	void *m_editp;
	int m_scale;
	int m_rate;
	int m_frame;
	int m_numFrame;
	char m_strTime[STRLEN];
	char m_strHis[NUMHIS][STRLEN];
	int m_numHis;
	bool m_loadfile;
	int m_autosave;
	int m_listBoxHeight;
	int m_timelineHeight;
	int m_thumbWidth;
	int m_thumbHeight;
	int m_thumbAspect;
	int m_thumbsNum;
	HBITMAP m_hbmThumbs[NUMTHUMBS];
	WNDPROC m_pStaticWndProc;
	int m_hoveredChapter;
	bool m_mouseTracking;

	int m_numChapter;
	int m_numChapterCapacity;
	int *m_Frame;
	char (*m_strTitle)[STRLEN];
	//[xt]m_SCPosの情報はm_strTitleに含まれていて冗長なので削除してみる

	void AddHis();
	bool SaveToFile(const char *lpFile);

	//[xt]関数追加
	void CreateUndoPoint();
	void ClearUndoPoint();
	void ReserveChapterList(int numCapacity);
	void FreeChapterList();
	void UpdateThumbs();
	void ClearThumbs();
	int FindNearestChapter(int num, int denum, int threshold) const;
	static LRESULT CALLBACK TimelineWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

public:
	CfgDlg() : m_hDlg(NULL) {}
	void ShowList(int defSelect = -1);
	void Init(HWND hwnd,void *editp,FILTER *fp);
	bool IsInit() const { return m_hDlg != NULL; }
	void Exit();
	void Resize();
	void SetFrame(int frame);
	void SetFps(int rate,int scale);
	void SetFrameN(void *editp,int frame_n);
	void AddList();
	void DelList();
	void NextList();
	void PrevList();
	void NextHereList();
	void PrevHereList();
	void Seek();
	void Save();
	void AutoSave();
	void Load();
	void AutoSaveCheck();
	int GetSCPos(int moveto, int frames);

	//[ru]関数追加
	void LoadFromFile(char *filename);
	void DetectMute();
	//ここまで

	void UpdateFramePos();

	//[xt]関数追加
	void UpdateDlgItems();
	void DelMuteChapters();
	void ExpandEditPointChapters();
	bool SetupThumbs();
	void ProjectSave(void *data, int *size) const;
	void ProjectLoad(const void *data, int size);
};

#endif
