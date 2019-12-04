#ifndef FFStreamRecordTask_H
#define FFStreamRecordTask_H


class FFStreamRecordTask
{
public:
    FFStreamRecordTask();
    virtual ~FFStreamRecordTask();

	void  SetInputUrl(string rtspUrl);
	void  SetOutputPath(string outputPath);

    void  StartRecvStream();
    void  StopRecvStream();

	void GetVideoSize(long & width, long & height)
	{
		width  = m_video_width;
		height = m_video_height;
	}

	void  SetNotifyWnd(HWND hNotify)
	{
		m_hNotifyWnd = hNotify;
	}
		
	BOOL   CheckTimeOut(DWORD dwCurrentTime);

private:
    void run();

    int    OpenInputStream();
    void   CloseInputStream();

    void   readAndMux();

	static DWORD WINAPI ReadingThrd(void * pParam);

	int   openOutputStream();
	void  closeOutputStream();


private:

    string m_InputUrl;
    string m_outputFile;

    AVFormatContext* m_inputAVFormatCxt;
    AVBitStreamFilterContext* m_bsfcAAC;
	AVBitStreamFilterContext* m_bsfcH264;

	int m_videoStreamIndex;
	int m_audioStreamIndex;

    AVFormatContext* m_outputAVFormatCxt;

    char m_tmpErrString[64];
    bool m_stop_status;

	HANDLE m_hReadThread;

	BOOL   m_bInputInited;
	BOOL   m_bOutputInited;

	int    m_video_width, m_video_height;
	int    m_frame_rate;

	HWND    m_hNotifyWnd;

	DWORD     m_dwStartConnectTime; //开始接收的时间，系统时间，单位：毫秒
	DWORD     m_dwLastRecvFrameTime; //上一次收到帧数据的时间，系统时间，单位：毫秒
	DWORD     m_nMaxRecvTimeOut; //网络接收数据的超时时间，单位：毫秒
	DWORD     m_nMaxConnectTimeOut; //连接超时，单位：毫秒

	int       m_nRecordedTimeLen; //录制的总秒数

};

#endif // FFStreamRecordTask_H
