// MainFrm.cpp : CMainFrame 类的实现
//

#include "stdafx.h"
#include "StreamRecord.h"
#include <mmsystem.h>
#include "MainFrm.h"
#include "PProfile.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#endif



#define  WM_UPDATE_CLIENT_STATE  WM_USER + 102

#define  WM_START_STREAM         WM_USER + 104
#define  WM_STOP_STREAM          WM_USER + 105



#define  WM_SET_BORDER_STYLE      WM_USER + 21


time_t m_tStarted = 0;
UINT   m_uBytesWritten = 0;
ULONG  gChannelTotalLength;
ULONG  StartTime;
BOOL   g_border = 1;


CMainFrame         *  gpMainFrame = NULL;


// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	//ON_WM_SETFOCUS()
	ON_WM_TIMER()
	ON_WM_DESTROY()
    ON_COMMAND_RANGE(4770,4850, OnClickMenu)
    ON_UPDATE_COMMAND_UI_RANGE(4770, 4850, OnUpdateMenuUI)
	ON_MESSAGE(WM_COMM_MESSAGE, OnProcessCommnMessage)
	ON_MESSAGE(WM_START_STREAM, OnStartReceiveStream)
	ON_MESSAGE(WM_STOP_STREAM, OnStopReceiveStream)
	ON_WM_SIZE()
	ON_WM_MOVE()
	ON_WM_MOVING()
END_MESSAGE_MAP()


BOOL GetHostInfo(char * outIP, char * outName)
{
	char   name[256];
	if (gethostname(name, 256) == 0)
	{
		if (outName)
		{
			strcpy(outName,  name);
		}

		PHOSTENT  hostinfo;
		if ((hostinfo = gethostbyname(name)) != NULL)
		{
			LPCSTR pIP = inet_ntoa (*(struct in_addr *)*hostinfo->h_addr_list);
			strcpy(outIP,  pIP);
			return TRUE;
		}
	}
	return FALSE;
}

// CMainFrame 构造/析构

CMainFrame::CMainFrame()
{
	gpMainFrame = this;

	memset(m_szFilePath, 0, sizeof(m_szFilePath));
   
	m_nFPS = 0;
		                 
	m_fp = INVALID_HANDLE_VALUE;
	m_bRecording = FALSE; 

	 /* register all codecs, demux and protocols */
    avcodec_register_all();
    av_register_all();
}

CMainFrame::~CMainFrame()
{

}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndView.Create(NULL, NULL, AFX_WS_DEFAULT_VIEW,
		CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL))
	{
		TRACE0("未能创建视图窗口\n");
		return -1;
	}

	SetWindowPos(NULL, 0, 0, 400, 350, SWP_NOZORDER|SWP_NOMOVE);


	//_tcscpy(m_szFilePath, "rtsp://admin:123456@192.168.1.102/videoMain");
	//OnStartReceiveStream(0, 0);

	return 0;
}

void CMainFrame::OnDestroy()
{

    OnStopReceiveStream(0, 0);

	Sleep(100);
	
	CFrameWnd::OnDestroy();
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;

	// TODO: 在此处通过修改 CREATESTRUCT cs 来修改窗口类或样式

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);
	cs.style |= WS_CLIPCHILDREN;

	return TRUE;
}


// CMainFrame 诊断

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG



void CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
	// 将焦点前移到视图窗口
	//m_wndView.SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	//if (m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
	//	return TRUE;

	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}



void CMainFrame::OnBnConnect()
{
	//int iRes = 0;
	//CString  strFilter = "";
	//strFilter += "Video File (*.h264);(*.264);(*.3gp);(*.mp4);(*.ts)|*.h264;*.264;*.3gp;*.mp4;*.ts|";
	//strFilter += "All Files (*.*)|*.*|";

	//CFileDialog dlgOpen(TRUE, NULL, NULL, OFN_PATHMUSTEXIST | OFN_HIDEREADONLY, strFilter, this);
	//if (IDOK != dlgOpen.DoModal()) 
	//{
	//    return;
	//}

	//_tcscpy(m_szFilePath, dlgOpen.GetPathName());


	OnStopReceiveStream(0, 0);
	OnStartReceiveStream(0, 0);
}

void CMainFrame::OnBnDisconnect()
{
	OnStopReceiveStream(0, 0);

}

void CMainFrame::OnClickMenu(UINT uMenuId)
{
	// Parse the menu command
	switch (uMenuId)
	{
		case IDM_CONNECT: //连接
		{
			 OnBnConnect();
		}
		break;
		
		case IDM_DISCONNECT: //断开连接
		{
            OnBnDisconnect();  
		}
		break;
		

		//case IDM_OSD1:
		//case IDM_OSD2:
		//case IDM_OSD3:
		//	{
		//		HandleMenuID(uMenuId, m_ptMenuClickPoint);
		//	}
		//	break;

	}//switch	 
}


void CMainFrame:: OnUpdateMenuUI(CCmdUI* pCmdUI)
{
  switch(pCmdUI->m_nID)
 {
  case IDM_CONNECT:
	  {	 
		//pCmdUI->Enable(1); 
	  }
     break;
  case IDM_DISCONNECT:
	  {
		//pCmdUI->Enable(1);	 
	  }
	 break;
  case IDM_SET_NOBORDER:
	  {
        pCmdUI->SetCheck(!g_border);
	  }
	  break;


 }//switch

}

static  BOOL g_bCaptureCursor = FALSE;
POINT   g_pCapturePt;


// We use  "Esc" key to restore from fullscreen mode
BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) 
{
	
	switch(pMsg->message)
	{
 
		case WM_LBUTTONDBLCLK:
		{
			//CRect rcVideo;
			//::GetWindowRect(m_Decoder.m_hWnd, rcVideo);
			////ScreenToClient(rcVideo);

			//POINT pt;
			//::GetCursorPos(&pt);
			//if (rcVideo.PtInRect(pt))
			//{
			//  ::SendMessage(m_hWnd, WM_SHOW_MAXIMIZED, 0, 0);
			//  m_bFullScreen = TRUE;
			//}	
			//    
			//return TRUE;

		}
		break;
		
		case WM_RBUTTONDOWN:
			{
				LONG style = GetWindowLong(m_hWnd, GWL_STYLE);
				if(style & (WS_DLGFRAME | WS_THICKFRAME | WS_BORDER))
				{
					CMenu menu;
					menu.CreatePopupMenu();
					menu.AppendMenu(MF_STRING, IDM_OSD1, "Test1");
					menu.AppendMenu(MF_STRING, IDM_OSD2, "Test2");
					menu.AppendMenu(MF_STRING, IDM_OSD3, "Test3");

					POINT pt;
					::GetCursorPos(&pt);
					menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this);

	                ScreenToClient(&pt);
					m_ptMenuClickPoint = pt;
				}

			}
			break;
			
		case WM_LBUTTONDOWN:
		{
			LONG style = GetWindowLong(m_hWnd, GWL_STYLE);
			if(style & (WS_DLGFRAME | WS_THICKFRAME | WS_BORDER))
			{
				return TRUE;
			}

			SetCapture();
			g_bCaptureCursor = TRUE;
			//TRACE("SetCapture() \n");
				 
			POINT pt;
			pt = pMsg->pt;
			ScreenToClient(&pt);

			int nCaptionHeight = 0;
			if(g_border)
				nCaptionHeight = GetSystemMetrics(SM_CYSMCAPTION);

			g_pCapturePt.x   = pt.x;
			g_pCapturePt.y   = pt.y + nCaptionHeight;

		}
		break;

		case WM_LBUTTONUP:
		{
			ReleaseCapture();
			g_bCaptureCursor = FALSE;
			//TRACE("ReleaseCapture() \n");
		}
			break;

		case WM_MOUSEMOVE:
		{
			if(!g_bCaptureCursor)
				return TRUE;

				SetWindowPos(NULL, 
					pMsg->pt.x - g_pCapturePt.x,
					pMsg->pt.y - g_pCapturePt.y, 
					0, 0, SWP_NOZORDER|SWP_NOSIZE);
			  
			return TRUE;
		}
			break;

		case WM_KEYDOWN:
		{
			if(::GetAsyncKeyState(VK_F5))
			{
				//int nTitleHeight  = GetSystemMetrics(SM_CYCAPTION);
				//int nBorderWidth  = GetSystemMetrics(SM_CXBORDER);
				//int nBorderHeight = GetSystemMetrics(SM_CYBORDER);

				RECT rcWin, rcClient;
				GetWindowRect(&rcWin);
				GetClientRect(&rcClient);

				int nBorderWidth = (rcWin.right-rcWin.left) - (rcClient.right - rcClient.left);
				int nBorderHeight = (rcWin.bottom - rcWin.top) - (rcClient.bottom - rcClient.top);

				//CSize size;
	           //GetVideoSize(size);

				//if(size.cx > 0 && size.cy > 0)
				//{
				//	SetWindowPos(NULL, 0, 0, size.cx+nBorderWidth, size.cy+nBorderHeight, SWP_NOMOVE);
				//}
			}
	       
		}
		  break;

	}//switch

	return CFrameWnd::PreTranslateMessage(pMsg);
}



void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);

}


void CMainFrame::OnMove(int x, int y)
{
	CFrameWnd::OnMove(x, y);
	//TRACE("OnMove() \n");
}

void CMainFrame::OnMoving(UINT fwSide, LPRECT pRect)
{
	CFrameWnd::OnMoving(fwSide, pRect);
	//TRACE("OnMoveing() \n");
}


LRESULT CMainFrame::OnProcessCommnMessage(WPARAM wParam, LPARAM lParam)
{
	if(wParam == 0)
	{
		::MessageBox(m_hWnd, _T("录制文件成功"), _T("提示"), MB_OK);

		return 0;
	}
	else if(wParam == 1)
	{
		TRACE("收到退出进程命令: exit = 1\n");
		OnStopReceiveStream(0, 0);
		::PostQuitMessage(0);
	}

	return 0;
}


void CMainFrame::OnTimer(UINT nIDEvent)
{
	CFrameWnd::OnTimer(nIDEvent);

}

void   CMainFrame::HandleMenuID(UINT  nOSDMenuID, POINT pt)
{
	switch(nOSDMenuID)
	{
	case IDM_OSD1:
		break;
	case IDM_OSD2:
		break;
	case IDM_OSD3:
		break;
	}
}

LRESULT  CMainFrame:: OnStartReceiveStream(WPARAM wParam, LPARAM lParam)
{
	BOOL pass = FALSE;

	char szFilePath[256] = {0};
	P_GetProfileString(_T("Client"), "file_path", szFilePath, sizeof(szFilePath)); //保存的文件路径
	
	char szURL[256] = {0};
	P_GetProfileString(_T("Client"), "URL", szURL, sizeof(szURL));

	//m_StreamRecorder.SetNotifyWnd(m_hWnd);

	m_StreamRecorder.SetInputUrl(szURL); //设置要打开的Rtsp URL
	m_StreamRecorder.SetOutputPath(szFilePath); //设置输出文件路径

	m_StreamRecorder.StartRecvStream();

	StartTime = timeGetTime();
	gChannelTotalLength = 0;
	m_frmCount = 0;
	m_nFPS = 0;

	return (pass == TRUE)?0:1;
}

LRESULT  CMainFrame:: OnStopReceiveStream(WPARAM wParam, LPARAM lParam)
{
	m_StreamRecorder.StopRecvStream();
	Sleep(50);

	StartTime = 0;
	gChannelTotalLength = 0;

	m_bRecording = FALSE;

	return 0;
} 


