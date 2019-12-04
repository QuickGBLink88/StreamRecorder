// MainFrm.h : CMainFrame 类的接口
//


#pragma once

#include "ChildView.h"
#include <afxmt.h>
#include "VideoDisplayWnd.h"
//#include "H264.h"
#include "FFStreamRecordTask.h"

#include <map>
using namespace std;


class CMainFrame : public CFrameWnd
{
	
public:
	CMainFrame();
    virtual ~CMainFrame();

public:



protected: 
	DECLARE_DYNAMIC(CMainFrame)

	LRESULT  OnVideoWinMsg(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);

   	void     SetServerIP(long  inServerIP);
	
	LRESULT  OnStartReceiveStream(WPARAM wParam, LPARAM lParam);
	LRESULT  OnStopReceiveStream(WPARAM wParam, LPARAM lParam);

	 void OnBnConnect();
	 void OnBnDisconnect();

	 void OnTimer(UINT nIDEvent);
	 void OnDestroy();

	 void OnStatisticsView();

	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	void   HandleMenuID(UINT  nOSDMenuID, POINT pt);

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	CChildView    m_wndView;

	FFStreamRecordTask  m_StreamRecorder;


protected:

	RECT              m_paintRect;
	CRect             m_rcWin;
	CRect             m_rcVideo;

	POINT              m_ptMenuClickPoint;

	UINT              m_frmCount;
    UINT              m_nFPS;

	HANDLE            m_fp;
	BOOL              m_bRecording; //录制文件

	char              m_szFilePath[256];

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClickMenu(UINT id);
	afx_msg void OnUpdateMenuUI(CCmdUI* pCmdUI);

	virtual BOOL PreTranslateMessage(MSG* pMsg);

	afx_msg void OnMove(int x, int y);
	afx_msg void OnMoving(UINT fwSide, LPRECT pRect);

	LRESULT OnProcessCommnMessage(WPARAM wParam, LPARAM lParam);

};


