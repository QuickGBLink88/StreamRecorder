#include "stdafx.h"
#include "FFStreamRecordTask.h"
#include <sstream>


//////////////////////////////////////////////////////////////

FFStreamRecordTask::FFStreamRecordTask()
{
	m_hNotifyWnd = NULL;
    m_stop_status = false;
    m_inputAVFormatCxt = nullptr;
    m_bsfcAAC = nullptr;
	m_bsfcH264 = nullptr;
    m_videoStreamIndex = -1;
	m_audioStreamIndex = -1;
    m_outputAVFormatCxt = nullptr;
	m_hReadThread = NULL;
	m_bInputInited = FALSE;
	m_bOutputInited = FALSE;
    m_video_width = m_video_height = 0;
    m_frame_rate = 25;

	m_dwStartConnectTime = 0;
	m_dwLastRecvFrameTime = 0;
	m_nMaxConnectTimeOut = 10000;
	m_nMaxRecvTimeOut = 50000;
	m_nRecordedTimeLen = 0;
}

FFStreamRecordTask::~FFStreamRecordTask()
{
	StopRecvStream();
}


void  FFStreamRecordTask::SetInputUrl(string rtspUrl)
{
	m_InputUrl = rtspUrl;
}

void  FFStreamRecordTask::SetOutputPath(string outputPath)
{
	m_outputFile = outputPath;
}

void FFStreamRecordTask::StartRecvStream()
{
	if(m_InputUrl.empty())
		return;

    m_videoStreamIndex = -1;
	m_audioStreamIndex = -1;

	m_bInputInited  = FALSE;
	m_bOutputInited = FALSE;

	m_video_width = m_video_height = 0;
    m_nRecordedTimeLen = 0;

   	DWORD threadID = 0;
	m_hReadThread = CreateThread(NULL, 0, ReadingThrd, this, 0, &threadID);
}


void FFStreamRecordTask::StopRecvStream()
{
    m_stop_status = true;

	if (m_hReadThread != NULL) 
	{
        WaitForSingleObject(m_hReadThread, INFINITE);
		CloseHandle(m_hReadThread);
		m_hReadThread = NULL;
	}
    CloseInputStream();
}

DWORD WINAPI FFStreamRecordTask::ReadingThrd(void * pParam)
{
	FFStreamRecordTask * pTask = (FFStreamRecordTask *) pParam;

	pTask->run();

	OutputDebugString("ReadingThrd exited\n");

	return 0;
}

void FFStreamRecordTask::run()
{
    try
    {
		do
		{

			m_stop_status = false;

			if(OpenInputStream() != 0)
			{
				break;
			}
			if(openOutputStream() != 0)
			{
				break;
			}

			readAndMux();

			CloseInputStream();
			closeOutputStream();

		}while(0);
		
    }
    catch(std::exception& e)
    {
		TRACE("Exception Error: %s \n", e.what());
        //CloseInputStream();
		//closeOutputStream();
    }
}



static int interruptCallBack(void *ctx)
{
    FFStreamRecordTask * pSession = (FFStreamRecordTask*) ctx;

    if(pSession->CheckTimeOut(GetTickCount()))
    {
      return 1;
    }

   //continue 
   return 0;

}

BOOL   FFStreamRecordTask::CheckTimeOut(DWORD dwCurrentTime)
{
	if(m_stop_status)
		return TRUE;

	if(m_bInputInited)
	{
		if(m_dwLastRecvFrameTime > 0)
		{
			if((dwCurrentTime - m_dwLastRecvFrameTime) > m_nMaxRecvTimeOut) //接收过程中超时
			{
				OutputDebugString("Recv Timeout! \n");
				return TRUE;
			}
		}
	}
	else
	{
		if((dwCurrentTime - m_dwStartConnectTime) > m_nMaxConnectTimeOut) //连接超时
		{
			OutputDebugString("Connect Timeout! \n");
			return TRUE;
		}
	}
	return FALSE;
}


int FFStreamRecordTask::OpenInputStream()
{
    if (m_inputAVFormatCxt)
    {
        TRACE("already has input avformat \n");
		return -1;
    }

	///////////////////////////////////////////////////////////

    int res = 0;

    bool bIsNetPath = false;
	bool bIsRTSP = false;

	if(_strnicmp(m_InputUrl.c_str(), "rtsp://", 7) == 0)
	{
		bIsNetPath = true;
		bIsRTSP = true;
	}
	else if(_strnicmp(m_InputUrl.c_str(), "rtmp://", 7) == 0)
	{
		bIsNetPath = true;
	}
	else if(_strnicmp(m_InputUrl.c_str(), "http://", 7) == 0 
		|| _strnicmp(m_InputUrl.c_str(), "https://", 8) == 0 
		)
	{
		bIsNetPath = true;
	}
	else
	{
		bIsNetPath = false;
	}

	if(bIsNetPath) //从网络接收
	{
		//Initialize format context
		m_inputAVFormatCxt = avformat_alloc_context();

		//Initialize intrrupt callback
		AVIOInterruptCB icb = {interruptCallBack,this};
		m_inputAVFormatCxt->interrupt_callback = icb;
	}


	m_dwLastRecvFrameTime = 0;
	m_dwStartConnectTime = GetTickCount();

	//m_inputAVFormatCxt->flags |= AVFMT_FLAG_NONBLOCK;

    AVDictionary* options = nullptr;   
    if(bIsRTSP) 
		av_dict_set(&options, "rtsp_transport", "tcp", 0); 
    av_dict_set(&options, "stimeout", "9000000", 0);  //设置超时断开连接时间  


	DWORD dwTick1 = GetTickCount();
	DWORD dwTick2 = 0;

    res = avformat_open_input(&m_inputAVFormatCxt, m_InputUrl.c_str(), 0, &options);
    
	dwTick2 = GetTickCount();

	TRACE("avformat_open_input takes time: %ld ms \n", dwTick2 - dwTick1);

    if(res < 0)
    {
		TRACE("can not open input: %s, res: %d \n", m_InputUrl.c_str(), res);
		return -1;
    }

	m_inputAVFormatCxt->max_analyze_duration = 2000000;
	m_inputAVFormatCxt->probesize = 32*1024;
    //m_inputAVFormatCxt->fps_probe_size = 10;

	m_dwStartConnectTime = GetTickCount();

	TRACE("max_analyze_duration: %d, probesize: %d, fps_probe_size: %d \n", 
		m_inputAVFormatCxt->max_analyze_duration, m_inputAVFormatCxt->probesize, m_inputAVFormatCxt->fps_probe_size);

    if (avformat_find_stream_info(m_inputAVFormatCxt, 0) < 0)
    {
        TRACE("can not find stream info \n");
		return -1;
    }
    av_dump_format(m_inputAVFormatCxt, 0, m_InputUrl.c_str(), 0);
    for (int i = 0; i < m_inputAVFormatCxt->nb_streams; i++)
    {
        AVStream *in_stream = m_inputAVFormatCxt->streams[i];

		if (in_stream->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			m_videoStreamIndex = i; 

			m_video_width = in_stream->codec->width;
			m_video_height = in_stream->codec->height;

			if(in_stream->avg_frame_rate.den != 0 && in_stream->avg_frame_rate.num != 0)
			{
			  m_frame_rate = in_stream->avg_frame_rate.num/in_stream->avg_frame_rate.den;
			}

			TRACE("video stream index: %d, width: %d, height: %d, FrameRate: %d\n", m_videoStreamIndex, in_stream->codec->width, in_stream->codec->height, m_frame_rate);
		}
		else if (in_stream->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			m_audioStreamIndex = i;
		}
    }

	 //m_bsfcAAC = av_bitstream_filter_init("aac_adtstoasc");
  //  if(!m_bsfcAAC)
  //  {
  //      TraceT("can not create aac_adtstoasc filter \n");
  //  }

	//m_bsfcH264 = av_bitstream_filter_init("h264_mp4toannexb");
 //   if(!m_bsfcH264)
 //   {
 //       TRACE("can not create h264_mp4toannexb filter \n");
 //   }

	m_bInputInited = TRUE;

	return 0;
}

void FFStreamRecordTask::CloseInputStream()
{
    if (m_inputAVFormatCxt)
    {
        avformat_close_input(&m_inputAVFormatCxt);
    }
 //   if(m_bsfcAAC)
 //   {
 //       av_bitstream_filter_close(m_bsfcAAC);
 //       m_bsfcAAC = nullptr;
 //   }
	//if(m_bsfcH264)
	//{
	//    av_bitstream_filter_close(m_bsfcH264);
 //       m_bsfcH264 = nullptr;
	//}

	m_bInputInited = FALSE;
}


int  FFStreamRecordTask::openOutputStream()
{
    if (m_outputAVFormatCxt)
    {
        TRACE("already has rtmp avformat \n");
		return -1;
    }

    int res = 0;
    if(!m_outputFile.empty())
    {
		res = avformat_alloc_output_context2(&m_outputAVFormatCxt, NULL, NULL, m_outputFile.c_str()); 

        if (m_outputAVFormatCxt == NULL)
        {
            TRACE("can not alloc output context \n");
			return -1;
        }
		
        AVOutputFormat* fmt = m_outputAVFormatCxt->oformat;

  //      fmt->audio_codec = AV_CODEC_ID_AAC;
  //      fmt->video_codec = AV_CODEC_ID_H264;

        for (int i = 0; i < m_inputAVFormatCxt->nb_streams; i++)
        {
            AVStream *in_stream = m_inputAVFormatCxt->streams[i];

            AVStream *out_stream = avformat_new_stream(m_outputAVFormatCxt, in_stream->codec->codec);
            if (!out_stream)
            {
                TRACE("can not new out stream");
				return -1;
            }
            res = avcodec_copy_context(out_stream->codec, in_stream->codec);
            if (res < 0)
            {
                string strError = "can not copy context, url: " + m_InputUrl +  ",err msg:" + av_make_error_string(m_tmpErrString, AV_ERROR_MAX_STRING_SIZE, res);
				TRACE("%s \n", strError.c_str());
				return -1;
            }
            if (m_outputAVFormatCxt->oformat->flags & AVFMT_GLOBALHEADER)
            {
                out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
            }
        }
        av_dump_format(m_outputAVFormatCxt, 0, m_outputFile.c_str(), 1);
        if (!(fmt->flags & AVFMT_NOFILE))
        {
            res = avio_open(&m_outputAVFormatCxt->pb, m_outputFile.c_str(), AVIO_FLAG_WRITE);
            if (res < 0)
            {
                string strError = "can not open output io, file:" + m_outputFile + ", err msg:" + av_make_error_string(m_tmpErrString, AV_ERROR_MAX_STRING_SIZE, res);
				TRACE("%s \n", strError.c_str());
				return -1;
			}
        }


		res = avformat_write_header(m_outputAVFormatCxt, NULL);
        if (res < 0)
        {
            string strError = "can not write outputstream header, URL:" + m_outputFile + ", err msg:" + av_make_error_string(m_tmpErrString, AV_ERROR_MAX_STRING_SIZE, res);
			TRACE("%s \n", strError.c_str());
			m_bOutputInited = FALSE;
			return -1;
        }

		m_bOutputInited = TRUE;
    }
	return 0;
}

void FFStreamRecordTask::closeOutputStream()
{
    if (m_outputAVFormatCxt)
    {
        if(m_bOutputInited)
		{
          int res = av_write_trailer(m_outputAVFormatCxt); 
		}

        if (!(m_outputAVFormatCxt->oformat->flags & AVFMT_NOFILE))
        {
            if(m_outputAVFormatCxt->pb)
            {
                avio_close(m_outputAVFormatCxt->pb);
            }
        }

        avformat_free_context(m_outputAVFormatCxt);
        m_outputAVFormatCxt = nullptr;
    }
	m_bOutputInited = FALSE;
}



void FFStreamRecordTask::readAndMux()
{
	int nVideoFramesNum = 0;
	int64_t  first_pts_time = 0;

	DWORD start_time = GetTickCount(); 

    AVPacket pkt;
	av_init_packet(&pkt);
 
    while(1)
    {
        if(m_stop_status == true)
        {
            break;
        }

        int res;
       
        res = av_read_frame(m_inputAVFormatCxt, &pkt);
        if (res < 0)  //读取错误或流结束
        {
			if(AVERROR_EOF == res)
			{
				TRACE("End of file \n");
			}
			else
			{
			    TRACE("av_read_frame() got error: %d \n", res);
			}

			break;  
        }

		AVStream *in_stream = m_inputAVFormatCxt->streams[pkt.stream_index];
        AVStream *out_stream = m_outputAVFormatCxt->streams[pkt.stream_index];

        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;

        if(in_stream->codec->codec_type != AVMEDIA_TYPE_VIDEO && in_stream->codec->codec_type != AVMEDIA_TYPE_AUDIO)
		{
			av_free_packet(&pkt);
			continue;
		}

		if(in_stream->codec->codec_type == AVMEDIA_TYPE_VIDEO)  //视频
		{
			nVideoFramesNum++;

			int nSecs = (__int64)pkt.pts*in_stream->time_base.num/(__int64)in_stream->time_base.den;
			TRACE("Frame time: %02d:%02d \n", nSecs/60, nSecs%60);

			m_nRecordedTimeLen = nSecs;

			// write the compressed frame to the output format
			int nError = av_interleaved_write_frame(m_outputAVFormatCxt, &pkt);
			if (nError != 0) 
			{
				char tmpErrString[AV_ERROR_MAX_STRING_SIZE] = {0};
				av_make_error_string(tmpErrString, AV_ERROR_MAX_STRING_SIZE, nError);

				TRACE("Error: %d while writing video frame, %s\n", nError, tmpErrString);
			}

		}
		 else if(in_stream->codec->codec_type == AVMEDIA_TYPE_AUDIO) //音频
		 {
			// write the compressed frame to the output format
			int nError = av_interleaved_write_frame(m_outputAVFormatCxt, &pkt);
			if (nError != 0) 
			{
				char tmpErrString[AV_ERROR_MAX_STRING_SIZE] = {0};
				av_make_error_string(tmpErrString, AV_ERROR_MAX_STRING_SIZE, nError);

				TRACE("Error: %d while writing audio frame, %s\n", nError, tmpErrString);
			}
		 }

		//if((in_stream->codec->codec_type == AVMEDIA_TYPE_VIDEO)	) //下面这段代码是为了控制流读取的速度，但是因为输入的是实时流，不用控制速度。
		//{
		//	if(first_pts_time == 0)
		//	    first_pts_time = pkt.pts;

		//	int64_t pts_time = (pkt.pts - first_pts_time)*1000*in_stream->time_base.num/in_stream->time_base.den; //转成毫秒
		//	int64_t now_time = GetTickCount() - start_time; 

		//	//if(pts_time > now_time + 10 && pts_time < now_time + 3000)
		//	//{
		//	//	Sleep(pts_time-now_time);
		//	//}
		//	//else if(pts_time == 0 && nVideoFramesNum > 1) 
		//	//{
		//	//	Sleep(20);
		//	//}
		//}

		av_free_packet(&pkt);

		m_dwLastRecvFrameTime = GetTickCount();

    }//while

	TRACE("Reading ended, read %d video frames \n", nVideoFramesNum);
}




