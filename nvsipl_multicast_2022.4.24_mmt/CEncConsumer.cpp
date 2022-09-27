// Copyright (c) 2022 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software, related documentation and any
// modifications thereto. Any use, reproduction, disclosure or distribution
// of this software and related documentation without an express license
// agreement from NVIDIA Corporation is strictly prohibited.

#include "CEncConsumer.hpp"
#include "nvmedia_image_nvscibuf.h"
#include "nvmedia_iep_nvscisync.h"

CEncConsumer::CEncConsumer(NvSciStreamBlock handle,
                               uint32_t uSensor,
                               NvSciStreamBlock queueHandle,
                               uint16_t encodeWidth,
                               uint16_t encodeHeight) :
    CConsumer("EncConsumer", handle, uSensor, queueHandle)
{
    m_encodeWidth = encodeWidth;
    m_encodeHeight = encodeHeight;

    m_encodedBytes = 0;
    m_pEncodedBuf = nullptr;

    NVM_SURF_FMT_DEFINE_ATTR(surfFormatAttrs_input);
    NVM_SURF_FMT_SET_ATTR_YUV(surfFormatAttrs_input, YUV, 420, SEMI_PLANAR, UINT, 8, BL);
    m_surfaceType = NvMediaSurfaceFormatGetType(surfFormatAttrs_input, NVM_SURF_FMT_ATTR_MAX);
}

SIPLStatus CEncConsumer::HandleClientInit(void)
{
    m_pDevice.reset(NvMediaDeviceCreate());
    PCHK_PTR_AND_RETURN(m_pDevice, "NvMediaDeviceCreate");

    auto status = InitEncoder();
    PCHK_STATUS_AND_RETURN(status, "InitEncoder");

    // string fileName = "multicast_enc" + to_string(m_uSensorId) + ".h264";
    // m_pOutputFile = fopen(fileName.c_str(), "ab");
    // PCHK_PTR_AND_RETURN(m_pOutputFile, "Open encoder output file");

    return NVSIPL_STATUS_OK;
}

SIPLStatus CEncConsumer::SetEncodeConfig(void)
{
    memset(&m_stEncodeConfigH264Params, 0, sizeof(NvMediaEncodeConfigH264));
    m_stEncodeConfigH264Params.h264VUIParameters =
            (NvMediaEncodeConfigH264VUIParams*) calloc(1, sizeof(NvMediaEncodeConfigH264VUIParams));
    CHK_PTR_AND_RETURN(m_stEncodeConfigH264Params.h264VUIParameters, "Alloc h264VUIParameters failed");
    m_stEncodeConfigH264Params.h264VUIParameters->timingInfoPresentFlag = 1;

    // Setting Up Config Params
    m_stEncodeConfigH264Params.features = NVMEDIA_ENCODE_CONFIG_H264_ENABLE_OUTPUT_AUD|NVMEDIA_ENCODE_CONFIG_H264_ENABLE_ULTRA_FAST_ENCODE;
    m_stEncodeConfigH264Params.gopLength = 16;
    m_stEncodeConfigH264Params.idrPeriod = 16;
    m_stEncodeConfigH264Params.repeatSPSPPS = NVMEDIA_ENCODE_SPSPPS_REPEAT_INTRA_FRAMES;
    m_stEncodeConfigH264Params.adaptiveTransformMode = NVMEDIA_ENCODE_H264_ADAPTIVE_TRANSFORM_DISABLE;
    m_stEncodeConfigH264Params.bdirectMode = NVMEDIA_ENCODE_H264_BDIRECT_MODE_DISABLE;
    m_stEncodeConfigH264Params.intraRefreshPeriod             = 0;
    m_stEncodeConfigH264Params.intraRefreshCnt                = 0;
    m_stEncodeConfigH264Params.motionPredictionExclusionFlags = 0;
    m_stEncodeConfigH264Params.numSliceCountMinus1            = 0;
    m_stEncodeConfigH264Params.disableDeblockingFilterIDC     = 0;
    m_stEncodeConfigH264Params.quality = NVMEDIA_ENCODE_QUALITY_L0;
    m_stEncodeConfigH264Params.initQP.qpInterP = 20;
    m_stEncodeConfigH264Params.initQP.qpInterB = 20;
    m_stEncodeConfigH264Params.initQP.qpIntra  = 20;
    m_stEncodeConfigH264Params.maxQP.qpInterP = 51;
    m_stEncodeConfigH264Params.maxQP.qpInterB = 51;
    m_stEncodeConfigH264Params.maxQP.qpIntra  = 51;
    m_stEncodeConfigH264Params.rcParams.rateControlMode = NVMEDIA_ENCODE_PARAMS_RC_CONSTQP;
    m_stEncodeConfigH264Params.rcParams.numBFrames = 0;
    m_stEncodeConfigH264Params.rcParams.params.cbr.averageBitRate = 8000000;
    m_stEncodeConfigH264Params.maxSliceSizeInBytes = m_stEncodeConfigH264Params.rcParams.params.cbr.averageBitRate / 30.0;
    m_stEncodeConfigH264Params.rcParams.params.cbr.vbvBufferSize = 0;
    m_stEncodeConfigH264Params.rcParams.params.cbr.vbvInitialDelay = 0;
    auto nvmediaStatus = NvMediaIEPSetConfiguration(m_pNvMIEP.get(), &m_stEncodeConfigH264Params);
    PCHK_NVMSTATUS_AND_RETURN(nvmediaStatus, "NvMediaIEPSetConfiguration failed");

    return NVSIPL_STATUS_OK;
}

SIPLStatus CEncConsumer::InitEncoder(void)
{
    PLOG_DBG("Setting encoder initialization params\n");
    NvMediaEncodeInitializeParamsH264 encoderInitParams;
    memset(&encoderInitParams, 0, sizeof(encoderInitParams));
    encoderInitParams.profile = NVMEDIA_ENCODE_PROFILE_AUTOSELECT;
    encoderInitParams.level = NVMEDIA_ENCODE_LEVEL_AUTOSELECT;
    encoderInitParams.encodeHeight = m_encodeHeight;
    encoderInitParams.encodeWidth = m_encodeWidth;
    encoderInitParams.useBFramesAsRef = 0;
    encoderInitParams.frameRateDen = 1;
    encoderInitParams.frameRateNum = 30;
    encoderInitParams.maxNumRefFrames = 1;
    encoderInitParams.enableExternalMEHints = NVMEDIA_FALSE;
    m_pNvMIEP.reset(NvMediaIEPCreate(m_pDevice.get(), // nvmedia device
                                   NVMEDIA_IMAGE_ENCODE_H264, // codec
                                   &encoderInitParams,	 // init params
                                   m_surfaceType, // surfaceType
                                   0, // maxInputBuffering
                                   0, // maxOutputBuffering
                                   NVMEDIA_ENCODER_INSTANCE_0)); // encoder instance
    PCHK_PTR_AND_RETURN(m_pNvMIEP, "NvMediaImageEncoderCreate");

    auto status = SetEncodeConfig();
    return status;
}

CEncConsumer::~CEncConsumer(void)
{
    LOG_DBG("CEncConsumer release.\n");

    for (auto i = 0U; i < MAX_PACKETS; i++) {
        if (m_images[i] == nullptr) {
            continue;
        }
        auto nvmStatus = NvMediaIEPImageUnRegister(m_pNvMIEP.get(), m_images[i]);
        if(nvmStatus != NVMEDIA_STATUS_OK) {
            PLOG_WARN("NvMediaIEPImageUnRegister failed: 0x%x\n", nvmStatus);
        }
        NvMediaImageDestroy(m_images[i]);
    }

    if(m_stEncodeConfigH264Params.h264VUIParameters) {
        free(m_stEncodeConfigH264Params.h264VUIParameters);
        m_stEncodeConfigH264Params.h264VUIParameters = nullptr;
    }

    if (m_pOutputFile != nullptr) {
        fflush(m_pOutputFile);
        fclose(m_pOutputFile);
    }
}

// Buffer setup functions
SIPLStatus CEncConsumer::SetDataBufAttrList(void) {
    NvSciBufType bufType = NvSciBufType_Image;
    NvSciBufAttrValAccessPerm perm = NvSciBufAccessPerm_Readonly;
    bool cpuaccess_flag = true;

    NvSciBufAttrKeyValuePair bufAttrs[] = {
        { NvSciBufGeneralAttrKey_Types, &bufType, sizeof(bufType) },
        { NvSciBufGeneralAttrKey_RequiredPerm, &perm, sizeof(perm) },
        { NvSciBufGeneralAttrKey_NeedCpuAccess, &cpuaccess_flag, sizeof(cpuaccess_flag) },
    };

    auto sciErr = NvSciBufAttrListSetAttrs(m_bufAttrLists[DATA_ELEMENT_INDEX], bufAttrs, sizeof(bufAttrs) / sizeof(NvSciBufAttrKeyValuePair));
    PCHK_NVSCISTATUS_AND_RETURN(sciErr, "NvSciBufAttrListSetAttrs");

    return NVSIPL_STATUS_OK;
}

// Sync object setup functions
SIPLStatus CEncConsumer::SetSyncAttrList(void)
{
    auto nvmStatus = NvMediaIEPFillNvSciSyncAttrList(m_pNvMIEP.get(), m_signalerAttrList, NVMEDIA_SIGNALER);
    PCHK_NVMSTATUS_AND_RETURN(nvmStatus, "Signaler NvMediaIEPFillNvSciSyncAttrList");

    nvmStatus = NvMediaIEPFillNvSciSyncAttrList(m_pNvMIEP.get(), m_waiterAttrList, NVMEDIA_WAITER);
    PCHK_NVMSTATUS_AND_RETURN(nvmStatus, "Waiter NvMediaIEPFillNvSciSyncAttrList");

    return NVSIPL_STATUS_OK;
}

SIPLStatus CEncConsumer::MapDataBuffer(uint32_t packetIndex)
{
    NvMediaStatus nvmStatus =
        NvMediaImageCreateFromNvSciBuf(m_pDevice.get(), m_packets[packetIndex].dataObj, &m_images[packetIndex]);
    PCHK_NVMSTATUS_AND_RETURN(nvmStatus, "NvMediaImageCreateFromNvSciBuf");

    nvmStatus = NvMediaIEPImageRegister(m_pNvMIEP.get(), m_images[packetIndex], NVMEDIA_ACCESS_MODE_READ);
    PCHK_NVMSTATUS_AND_RETURN(nvmStatus, "NvMediaIEPImageRegister");

    return NVSIPL_STATUS_OK;
}

SIPLStatus CEncConsumer::RegisterSignalSyncObj(void)
{
    auto nvmStatus = NvMediaIEPRegisterNvSciSyncObj(m_pNvMIEP.get(), NVMEDIA_EOFSYNCOBJ, m_signalSyncObj);
    PCHK_NVMSTATUS_AND_RETURN(nvmStatus, "NvMediaIEPRegisterNvSciSyncObj for EOF");

    return NVSIPL_STATUS_OK;
}

SIPLStatus CEncConsumer::RegisterWaiterSyncObj(uint32_t index)
{
    auto nvmStatus = NvMediaIEPRegisterNvSciSyncObj(m_pNvMIEP.get(), NVMEDIA_PRESYNCOBJ, m_waiterSyncObjs[index]);
    PCHK_NVMSTATUS_AND_RETURN(nvmStatus, "NvMediaIEPRegisterNvSciSyncObj for PRE");

    return NVSIPL_STATUS_OK;
}

SIPLStatus CEncConsumer::UnregisterSyncObjs(void)
{
    auto nvmStatus = NvMediaIEPUnregisterNvSciSyncObj(m_pNvMIEP.get(),  m_signalSyncObj);
    PCHK_NVMSTATUS_AND_RETURN(nvmStatus, "NvMediaIEPUnregisterNvSciSyncObj for EOF");

    for (uint32_t i = 0U; i  < m_numWaitSyncObj; i++) {
        auto nvmStatus = NvMediaIEPUnregisterNvSciSyncObj(m_pNvMIEP.get(), m_waiterSyncObjs[i]);
        PCHK_NVMSTATUS_AND_RETURN(nvmStatus, "NvMediaIEPUnregisterNvSciSyncObj for PRE");
    }

    return NVSIPL_STATUS_OK;
}

SIPLStatus CEncConsumer::InsertPrefence(uint32_t packetIndex, NvSciSyncFence &prefence)
{
    auto nvmStatus = NvMediaIEPInsertPreNvSciSyncFence(m_pNvMIEP.get(), &prefence);
    PCHK_NVMSTATUS_AND_RETURN(nvmStatus, "NvMediaIEPInsertPreNvSciSyncFence");

    return NVSIPL_STATUS_OK;
}

SIPLStatus CEncConsumer::SetEofSyncObj(void)
{
    auto nvmStatus = NvMediaIEPSetNvSciSyncObjforEOF(m_pNvMIEP.get(), m_signalSyncObj);
    PCHK_NVMSTATUS_AND_RETURN(nvmStatus, "NvMediaIEPSetNvSciSyncObjforEOF");

    return NVSIPL_STATUS_OK;
}

SIPLStatus CEncConsumer::EncodeOneFrame(NvMediaImage *pNvMediaImage, uint8_t **ppOutputBuffer, size_t *pNumBytes, NvSciSyncFence *pPostfence)
{
    NvMediaEncodePicParamsH264 encodePicParams;
    uint32_t uNumBytes = 0U;
    uint32_t uNumBytesAvailable = 0U;
    uint8_t *pBuffer = nullptr;

    //set one frame params, default = 0
    memset(&encodePicParams, 0, sizeof(NvMediaEncodePicParamsH264));
    //IPP mode
    encodePicParams.pictureType = NVMEDIA_ENCODE_PIC_TYPE_AUTOSELECT;
    encodePicParams.encodePicFlags = NVMEDIA_ENCODE_PIC_FLAG_OUTPUT_SPSPPS;
    encodePicParams.nextBFrames    = 0;
    auto nvmStatus = NvMediaIEPFeedFrame(m_pNvMIEP.get(),     // *encoder
                                 pNvMediaImage,               // *frame
                                 nullptr,                     // *sourceRect
                                 &encodePicParams,            // encoder parameter
                                 NVMEDIA_ENCODER_INSTANCE_0); // encoder instance
    PCHK_NVMSTATUS_AND_RETURN(nvmStatus, "NvMediaIEPFeedFrame");

    nvmStatus = NvMediaIEPGetEOFNvSciSyncFence(m_pNvMIEP.get(), m_signalSyncObj, pPostfence);
    PCHK_NVMSTATUS_AND_RETURN(nvmStatus, ": NvMediaIEPGetEOFNvSciSyncFence");

    bool bEncodeFrameDone = false;
    while(!bEncodeFrameDone) {
        NvMediaBitstreamBuffer bitstreams = {0};
        uNumBytesAvailable = 0U;
        uNumBytes = 0U;
        nvmStatus = NvMediaIEPBitsAvailable(m_pNvMIEP.get(),
                                         &uNumBytesAvailable,
                                         NVMEDIA_ENCODE_BLOCKING_TYPE_IF_PENDING,
                                         NVMEDIA_VIDEO_ENCODER_TIMEOUT_INFINITE);
        switch(nvmStatus) {
            case NVMEDIA_STATUS_OK:
                pBuffer = new (std::nothrow) uint8_t[uNumBytesAvailable];
                if (pBuffer == nullptr) {
                    PLOG_ERR("Out of memory\n");
                    return NVSIPL_STATUS_OUT_OF_MEMORY;
                }

                bitstreams.bitstream = pBuffer;
                bitstreams.bitstreamSize = uNumBytesAvailable;
                std::fill(pBuffer, pBuffer + uNumBytesAvailable, 0xE5);
                nvmStatus = NvMediaIEPGetBitsEx(m_pNvMIEP.get(),
                                             &uNumBytes,
                                             1U,
                                             &bitstreams,
                                             nullptr);
                if(nvmStatus != NVMEDIA_STATUS_OK && nvmStatus != NVMEDIA_STATUS_NONE_PENDING) {
                    PLOG_ERR("Error getting encoded bits\n");
                    free(pBuffer);
                    return NVSIPL_STATUS_ERROR;
                }

                if(uNumBytes != uNumBytesAvailable) {
                    PLOG_ERR("Error-byte counts do not match %d vs. %d\n",
                            uNumBytesAvailable, uNumBytes);
                    free(pBuffer);
                    return NVSIPL_STATUS_ERROR;
                }
                *ppOutputBuffer = pBuffer;
                *pNumBytes = (size_t)uNumBytesAvailable;
                bEncodeFrameDone = 1;
                break;

            case NVMEDIA_STATUS_PENDING:
                PLOG_DBG("Status - pending\n");
                break;

            case NVMEDIA_STATUS_NONE_PENDING:
                PLOG_ERR("Error - no encoded data is pending\n");
                return NVSIPL_STATUS_ERROR;

            default:
                PLOG_ERR("Error occured\n");
                return NVSIPL_STATUS_ERROR;
        }
    }

    return NVSIPL_STATUS_OK;
}

// Streaming functions
SIPLStatus CEncConsumer::ProcessPayload(uint32_t packetIndex, NvSciSyncFence *pPostfence)
{
    PLOG_DBG("Process payload (packetIndex = 0x%x).\n", packetIndex);

    auto status = EncodeOneFrame(m_images[packetIndex], &m_pEncodedBuf, &m_encodedBytes, pPostfence);
    PCHK_STATUS_AND_RETURN(status, "ProcessPayload");

    return NVSIPL_STATUS_OK;
}

bool CEncConsumer::ToSkipFrame(uint32_t frameNum)
{
    if (frameNum % 2U != 0) {
        return true;
    }

    return false;
}

SIPLStatus CEncConsumer::OnProcessPayloadDone(uint32_t packetIndex)
{
    SIPLStatus status = NVSIPL_STATUS_OK;

    //dump frames to local file
    if (m_frameNum <= DUMP_END_FRAME && m_frameNum >= DUMP_START_FRAME) {
        if (m_pOutputFile && m_pEncodedBuf && m_encodedBytes > 0) {
            if(fwrite(m_pEncodedBuf, m_encodedBytes, 1, m_pOutputFile) != 1) {
                PLOG_ERR("Error writing %d bytes\n", m_encodedBytes);
                status = NVSIPL_STATUS_ERROR;
                goto cleanup;
            }
            PLOG_DBG("writing %u bytes, m_frameNum %u\n", m_encodedBytes, m_frameNum);
            fflush(m_pOutputFile);
        }
    }
    PLOG_DBG("ProcessPayload succ.\n");

cleanup:
    if (m_pEncodedBuf) {
        free(m_pEncodedBuf);
        m_pEncodedBuf = nullptr;
    }
    m_encodedBytes = 0;

    return status;
}
