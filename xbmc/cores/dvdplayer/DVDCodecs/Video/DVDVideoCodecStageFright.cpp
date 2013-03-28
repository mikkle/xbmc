/*
 *      Copyright (C) 2010-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

//#define DEBUG_VERBOSE 1

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
#include "config.h"
#elif defined(_WIN32)
#include "system.h"
#endif

#if defined(HAVE_LIBSTAGEFRIGHT)
#include "DVDClock.h"
#include "settings/GUISettings.h"
#include "DVDStreamInfo.h"
#include "DVDVideoCodecStageFright.h"
#include "StageFrightVideo.h"
#include "utils/log.h"

#define CLASSNAME "CDVDVideoCodecStageFright"
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
CDVDVideoCodecStageFright::CDVDVideoCodecStageFright() 
  : CDVDVideoCodec()
  , m_stf_decoder(NULL), m_converter(NULL), m_convert_bitstream(false)
{
  m_pFormatName = "stf-xxxx";
}

CDVDVideoCodecStageFright::~CDVDVideoCodecStageFright()
{
  Dispose();
}

bool CDVDVideoCodecStageFright::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{  
  // we always qualify even if DVDFactoryCodec does this too.
  if (g_guiSettings.GetBool("videoplayer.usestagefright") && !hints.software)
  {
    m_convert_bitstream = false;
    CLog::Log(LOGDEBUG,
          "%s::%s - trying to open, codec(%d), profile(%d), level(%d)", 
          CLASSNAME, __func__, hints.codec, hints.profile, hints.level);

    switch (hints.codec)
    {
      case CODEC_ID_H264:
        m_pFormatName = "stf-h264";
        if (hints.extrasize < 7 || hints.extradata == NULL)
        {
          CLog::Log(LOGNOTICE,
              "%s::%s - avcC data too small or missing", CLASSNAME, __func__);
          return false;
        }
        m_converter     = new CBitstreamConverter();
        m_convert_bitstream = m_converter->Open(hints.codec, (uint8_t *)hints.extradata, hints.extrasize, true);

        break;
      case CODEC_ID_MPEG2VIDEO:
        m_pFormatName = "stf-mpeg2";
        break;
      case CODEC_ID_MPEG4:
        m_pFormatName = "stf-mpeg4";
        break;
      case CODEC_ID_VP3:
      case CODEC_ID_VP6:
      case CODEC_ID_VP6F:
      case CODEC_ID_VP8:
        m_pFormatName = "stf-vpx";
        break;
      case CODEC_ID_WMV3:
      case CODEC_ID_VC1:
        m_pFormatName = "stf-wmv";
        break;
      default:
        return false;
        break;
    }

    m_stf_decoder = new CStageFrightVideo;
    if (!m_stf_decoder->Open(hints))
    {
      CLog::Log(LOGERROR,
          "%s::%s - failed to open, codec(%d), profile(%d), level(%d)", 
          CLASSNAME, __func__, hints.codec, hints.profile, hints.level);
      delete m_stf_decoder;
      m_stf_decoder = NULL;
      
      if (m_converter)
      {
        m_converter->Close();
        delete m_converter;
        m_converter = NULL;
      }
      return false;
    }

    return true;
  }

  return false;
}

void CDVDVideoCodecStageFright::Dispose()
{
  if (m_converter)
  {
    m_converter->Close();
    delete m_converter;
    m_converter = NULL;
  }
  if (m_stf_decoder)
  {
    m_stf_decoder->Close();
    delete m_stf_decoder;
    m_stf_decoder = NULL;
  }
}

void CDVDVideoCodecStageFright::SetDropState(bool bDrop)
{
  m_stf_decoder->SetDropState(bDrop);
}

int CDVDVideoCodecStageFright::Decode(BYTE* pData, int iSize, double dts, double pts)
{
#if defined(DEBUG_VERBOSE)
  unsigned int time = XbmcThreads::SystemClockMillis();
#endif
  int rtn;
  int demuxer_bytes = iSize;
  uint8_t *demuxer_content = pData;

  if (m_convert_bitstream && demuxer_content)
  {
    // convert demuxer packet from bitstream to bytestream (AnnexB)
    if (m_converter->Convert(demuxer_content, demuxer_bytes))
    {
      demuxer_content = m_converter->GetConvertBuffer();
      demuxer_bytes = m_converter->GetConvertSize();
    } 
    else
      CLog::Log(LOGERROR,"%s::%s - bitstream_convert error", CLASSNAME, __func__);
  }
#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, ">>> decode conversion - tm:%d\n", XbmcThreads::SystemClockMillis() - time);
#endif

  rtn = m_stf_decoder->Decode(demuxer_content, demuxer_bytes, dts, pts);

  return rtn;
}

void CDVDVideoCodecStageFright::Reset(void)
{
  m_stf_decoder->Reset();
}

bool CDVDVideoCodecStageFright::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  return m_stf_decoder->GetPicture(pDvdVideoPicture);
}

bool CDVDVideoCodecStageFright::ClearPicture(DVDVideoPicture* pDvdVideoPicture)
{
  return m_stf_decoder->ClearPicture(pDvdVideoPicture);
}

void CDVDVideoCodecStageFright::SetSpeed(int iSpeed)
{
  m_stf_decoder->SetSpeed(iSpeed);
}

int CDVDVideoCodecStageFright::GetDataSize(void)
{
  return 0;
}

double CDVDVideoCodecStageFright::GetTimeSize(void)
{
  return 0;
}

#endif