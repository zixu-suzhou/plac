/*
 * Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

/* STL Headers */
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include <ctime>
#include <atomic>
#include <cmath>
#include <pthread.h>
#include <dlfcn.h>

/* NvSIPL Headers */
#include "NvSIPLVersion.hpp" // Version
#include "NvSIPLTrace.hpp" // Trace
#include "NvSIPLQueryTrace.hpp" // Query Trace
#include "NvSIPLCommon.hpp" // Common
#include "NvSIPLCamera.hpp" // Camera
#include "NvSIPLPipelineMgr.hpp" // Pipeline manager
#include "NvSIPLClient.hpp" // Client
#include "NvSIPLQuery.hpp" 
#include "CMaster.hpp"
#include "CProfiler.hpp"
#include "CCmdLineParser.hpp"
#include "platform/sf3324.hpp"
#include "platform/ar0820.hpp"

#define SECONDS_PER_ITERATION (2)
#define EVENT_QUEUE_TIMEOUT_US (1000000U)

constexpr unsigned long IMAGE_QUEUE_TIMEOUT_US = 1000000U;

/* Quit flag. */
std::atomic<bool> bQuit;

/* Ignore Error flag. */
std::atomic<bool> bIgnoreError;

/* SIPL Master. */
std::unique_ptr<CMaster> upMaster(nullptr);

/** Signal handler.*/
static void SigHandler(int signum)
{
    LOG_WARN("Received signal: %u. Quitting\n", signum);
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    bQuit = true;

    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
}

/** Sets up signal handler.*/
static void SigSetup(void)
{
    struct sigaction action { };
    action.sa_handler = SigHandler;

    sigaction(SIGINT, &action, nullptr);
    sigaction(SIGTERM, &action, nullptr);
    sigaction(SIGQUIT, &action, nullptr);
    sigaction(SIGHUP, &action, nullptr);
}

class CDeviceBlockNotificationHandler : public NvSIPLPipelineNotifier
{
public:
    uint32_t m_uDevBlkIndex = -1U;

    SIPLStatus Init(uint32_t uDevBlkIndex, DeviceBlockInfo &oDeviceBlockInfo,
                    INvSIPLNotificationQueue *notificationQueue)
    {
        if (notificationQueue == nullptr) {
            LOG_ERR("Invalid Notification Queue\n");
            return NVSIPL_STATUS_BAD_ARGUMENT;
        }
        m_uDevBlkIndex = uDevBlkIndex;
        m_oDeviceBlockInfo = oDeviceBlockInfo;
        m_pNotificationQueue = notificationQueue;

        SIPLStatus status = upMaster->GetMaxErrorSize(m_uDevBlkIndex, m_uErrorSize);
        if (status != NVSIPL_STATUS_OK) {
            LOG_ERR("DeviceBlock: %u, GetMaxErrorSize failed\n", m_uDevBlkIndex);
            return status;
        }

        if (m_uErrorSize != 0U) {
            m_oDeserializerErrorInfo.upErrorBuffer.reset(new uint8_t[m_uErrorSize]);
            m_oDeserializerErrorInfo.bufferSize = m_uErrorSize;

            m_oSerializerErrorInfo.upErrorBuffer.reset(new uint8_t[m_uErrorSize]);
            m_oSerializerErrorInfo.bufferSize = m_uErrorSize;

            m_oSensorErrorInfo.upErrorBuffer.reset(new uint8_t[m_uErrorSize]);
            m_oSensorErrorInfo.bufferSize = m_uErrorSize;
        }

        m_upThread.reset(new std::thread(EventQueueThreadFunc, this));
        return NVSIPL_STATUS_OK;
    }

    void Deinit()
    {
        m_bQuit = true;
        if (m_upThread != nullptr) {
            m_upThread->join();
            m_upThread.reset();
        }
    }

    bool IsDeviceBlockInError() {
        return m_bInError;
    }

    virtual ~CDeviceBlockNotificationHandler()
    {
        Deinit();
    }

private:
    void HandleDeserializerError()
    {
        bool isRemoteError {false};
        uint8_t linkErrorMask {0U};

        /* Get detailed error information (if error size is non-zero) and
         * information about remote error and link error. */
        SIPLStatus status = upMaster->GetDeserializerErrorInfo(
                                        m_uDevBlkIndex,
                                        (m_uErrorSize > 0) ? &m_oDeserializerErrorInfo : nullptr,
                                        isRemoteError,
                                        linkErrorMask);
        if (status != NVSIPL_STATUS_OK) {
            LOG_ERR("DeviceBlock: %u, GetDeserializerErrorInfo failed\n", m_uDevBlkIndex);
            m_bInError = true;
            return;
        }

        if ((m_uErrorSize > 0) && (m_oDeserializerErrorInfo.sizeWritten != 0)) {
            cout << "DeviceBlock[" << m_uDevBlkIndex << "] Deserializer Error Buffer: ";
            for (uint32_t i = 0; i < m_oDeserializerErrorInfo.sizeWritten; i++) {
                cout << m_oDeserializerErrorInfo.upErrorBuffer[i] << " ";
            }
            cout << "\n";

            m_bInError = true;
        }

        if (isRemoteError) {
            cout << "DeviceBlock[" << m_uDevBlkIndex << "] Deserializer Remote Error.\n";
            for (uint32_t i = 0; i < m_oDeviceBlockInfo.numCameraModules; i++) {
                HandleCameraModuleError(m_oDeviceBlockInfo.cameraModuleInfoList[i].sensorInfo.id);
            }
        }

        if (linkErrorMask != 0U) {
            LOG_ERR("DeviceBlock: %u, Deserializer link error. mask: %u\n", m_uDevBlkIndex, linkErrorMask);
            m_bInError = true;
        }
    }

    void HandleCameraModuleError(uint32_t index)
    {
        if (m_uErrorSize > 0) {
            /* Get detailed error information. */
            SIPLStatus status = upMaster->GetModuleErrorInfo(
                                            index,
                                            &m_oSerializerErrorInfo,
                                            &m_oSensorErrorInfo);
            if (status != NVSIPL_STATUS_OK) {
                LOG_ERR("index: %u, GetModuleErrorInfo failed\n", index);
                m_bInError = true;
                return;
            }

            if (m_oSerializerErrorInfo.sizeWritten != 0) {
                cout << "Pipeline[" << index << "] Serializer Error Buffer: ";
                for (uint32_t i = 0; i < m_oSerializerErrorInfo.sizeWritten; i++) {
                    cout << m_oSerializerErrorInfo.upErrorBuffer[i] << " ";
                }
                cout << "\n";
                m_bInError = true;
            }

            if (m_oSensorErrorInfo.sizeWritten != 0) {
                cout << "Pipeline[" << index << "] Sensor Error Buffer: ";
                for (uint32_t i = 0; i < m_oSensorErrorInfo.sizeWritten; i++) {
                    cout << m_oSensorErrorInfo.upErrorBuffer[i] << " ";
                }
                cout << "\n";
                m_bInError = true;
            }
        }
    }

    bool isTrueGPIOInterrupt(const uint32_t *gpioIdxs, uint32_t numGpioIdxs)
    {
        /*
         * Get disambiguated GPIO interrupt event codes, to determine whether
         * true interrupts or propagation functionality fault occurred.
         */

        bool true_interrupt = false;

        for (uint32_t i = 0U; i < numGpioIdxs; i++) {
            SIPLGpioEvent code;
            SIPLStatus status = upMaster->GetErrorGPIOEventInfo(m_uDevBlkIndex,
                                                                gpioIdxs[i],
                                                                code);
            if (status == NVSIPL_STATUS_NOT_SUPPORTED) {
                LOG_INFO("GetErrorGPIOEventInfo is not supported by OS backend currently!\n");
                /* Allow app to fetch detailed error info, same as in case of true interrupt. */
                return true;
            } else if (status != NVSIPL_STATUS_OK) {
                LOG_ERR("DeviceBlock: %u, GetErrorGPIOEventInfo failed\n", m_uDevBlkIndex);
                m_bInError = true;
                return false;
            }

            /*
             * If no error condition code is returned, and at least one GPIO has
             * NVSIPL_GPIO_EVENT_INTR status, return true.
             */
            if (code == NVSIPL_GPIO_EVENT_INTR) {
                true_interrupt = true;
            } else if (code != NVSIPL_GPIO_EVENT_NOTHING) {
                // GPIO functionality fault (treat as fatal)
                m_bInError = true;
                return false;
            }
        }

        return true_interrupt;
    }

    //! Notifier function
    void OnEvent(NotificationData &oNotificationData)
    {
        switch (oNotificationData.eNotifType) {
        case NOTIF_ERROR_DESERIALIZER_FAILURE:
            LOG_ERR("DeviceBlock: %u, NOTIF_ERROR_DESERIALIZER_FAILURE\n", m_uDevBlkIndex);
            if (!bIgnoreError) {
                if (isTrueGPIOInterrupt(oNotificationData.gpioIdxs, oNotificationData.numGpioIdxs)) {
                    HandleDeserializerError();
                }
            }
            break;
        case NOTIF_ERROR_SERIALIZER_FAILURE:
            LOG_ERR("DeviceBlock: %u, NOTIF_ERROR_SERIALIZER_FAILURE\n", m_uDevBlkIndex);
            if (!bIgnoreError) {
                for (uint32_t i = 0; i < m_oDeviceBlockInfo.numCameraModules; i++) {
                    if ((oNotificationData.uLinkMask & (1 << (m_oDeviceBlockInfo.cameraModuleInfoList[i].linkIndex))) != 0) {
                        if (isTrueGPIOInterrupt(oNotificationData.gpioIdxs, oNotificationData.numGpioIdxs)) {
                            HandleCameraModuleError(m_oDeviceBlockInfo.cameraModuleInfoList[i].sensorInfo.id);
                        }
                    }
                }
            }
            break;
        case NOTIF_ERROR_SENSOR_FAILURE:
            LOG_ERR("DeviceBlock: %u, NOTIF_ERROR_SENSOR_FAILURE\n", m_uDevBlkIndex);
            if (!bIgnoreError) {
                for (uint32_t i = 0; i < m_oDeviceBlockInfo.numCameraModules; i++) {
                    if ((oNotificationData.uLinkMask & (1 << (m_oDeviceBlockInfo.cameraModuleInfoList[i].linkIndex))) != 0) {
                        if (isTrueGPIOInterrupt(oNotificationData.gpioIdxs, oNotificationData.numGpioIdxs)) {
                            HandleCameraModuleError(m_oDeviceBlockInfo.cameraModuleInfoList[i].sensorInfo.id);
                        }
                    }
                }
            }
            break;
        case NOTIF_ERROR_INTERNAL_FAILURE:
            LOG_ERR("DeviceBlock: %u, NOTIF_ERROR_INTERNAL_FAILURE\n", m_uDevBlkIndex);
            m_bInError = true;
            break;
        default:
            LOG_WARN("DeviceBlock: %u, Unknown/Invalid notification\n", m_uDevBlkIndex);
            break;
        }
        return;
    }

    static void EventQueueThreadFunc(CDeviceBlockNotificationHandler *pThis)
    {
        SIPLStatus status = NVSIPL_STATUS_OK;
        NotificationData notificationData;

        if ((pThis == nullptr) || (pThis->m_pNotificationQueue == nullptr)) {
            LOG_ERR("Invalid thread data\n");
            return;
        }

        pthread_setname_np(pthread_self(), "DevBlkEvent");

        while (!pThis->m_bQuit) {
            status = pThis->m_pNotificationQueue->Get(notificationData, EVENT_QUEUE_TIMEOUT_US);
            if (status == NVSIPL_STATUS_OK) {
                pThis->OnEvent(notificationData);
            } else if (status == NVSIPL_STATUS_TIMED_OUT) {
                LOG_DBG("Queue timeout\n");
            } else if (status == NVSIPL_STATUS_EOF) {
                LOG_DBG("Queue shutdown\n");
                pThis->m_bQuit = true;
            } else {
                LOG_ERR("Unexpected queue return status\n");
                pThis->m_bQuit = true;
            }
        }
    }

    bool m_bQuit = false;
    bool m_bInError = false;
    std::unique_ptr<std::thread> m_upThread = nullptr;
    INvSIPLNotificationQueue *m_pNotificationQueue = nullptr;
    DeviceBlockInfo m_oDeviceBlockInfo;

    size_t m_uErrorSize {};
    SIPLErrorDetails m_oDeserializerErrorInfo {};
    SIPLErrorDetails m_oSerializerErrorInfo {};
    SIPLErrorDetails m_oSensorErrorInfo {};
};

class CPipelineNotificationHandler : public NvSIPLPipelineNotifier
{
public:
    uint32_t m_uSensor = -1U;

    //! Initializes the Pipeline Notification Handler
    SIPLStatus Init(uint32_t uSensor, INvSIPLNotificationQueue *notificationQueue)
    {
        if (notificationQueue == nullptr) {
            LOG_ERR("Invalid Notification Queue\n");
            return NVSIPL_STATUS_BAD_ARGUMENT;
        }
        m_uSensor = uSensor;
        m_pNotificationQueue = notificationQueue;
        m_upThread.reset(new std::thread(EventQueueThreadFunc, this));
        return NVSIPL_STATUS_OK;
    }

    void Deinit()
    {
        m_bQuit = true;
        if (m_upThread != nullptr) {
            m_upThread->join();
            m_upThread.reset();
        }
    }

    //! Returns true to pipeline encountered any fatal error.
    bool IsPipelineInError(void)
    {
        return m_bInError;
    }

    //! Get number of frame drops for a given sensor
    uint32_t GetNumFrameDrops(uint32_t uSensor)
    {
        if (uSensor >= MAX_NUM_SENSORS) {
            LOG_ERR("Invalid index used to request number of frame drops\n");
            return -1;
        }
        return m_uNumFrameDrops;
    }

    //! Reset frame drop counter
    void ResetFrameDropCounter(void)
    {
        m_uNumFrameDrops = 0U;
    }

    virtual ~CPipelineNotificationHandler()
    {
        Deinit();
    }

private:
    //! Notifier function
    void OnEvent(NotificationData &oNotificationData)
    {
        switch (oNotificationData.eNotifType) {
        case NOTIF_INFO_ICP_PROCESSING_DONE:
            LOG_INFO("Pipeline: %u, NOTIF_INFO_ICP_PROCESSING_DONE\n", oNotificationData.uIndex);
            break;
        case NOTIF_INFO_ISP_PROCESSING_DONE:
            LOG_INFO("Pipeline: %u, NOTIF_INFO_ISP_PROCESSING_DONE\n", oNotificationData.uIndex);
            break;
        case NOTIF_INFO_ACP_PROCESSING_DONE:
            LOG_INFO("Pipeline: %u, NOTIF_INFO_ACP_PROCESSING_DONE\n", oNotificationData.uIndex);
            break;
        case NOTIF_INFO_CDI_PROCESSING_DONE:
            LOG_INFO("Pipeline: %u, NOTIF_INFO_CDI_PROCESSING_DONE\n", oNotificationData.uIndex);
            break;
        case NOTIF_WARN_ICP_FRAME_DROP:
            LOG_WARN("Pipeline: %u, NOTIF_WARN_ICP_FRAME_DROP\n", oNotificationData.uIndex);
            m_uNumFrameDrops++;
            break;
        case NOTIF_WARN_ICP_FRAME_DISCONTINUITY:
            LOG_WARN("Pipeline: %u, NOTIF_WARN_ICP_FRAME_DISCONTINUITY\n", oNotificationData.uIndex);
            break;
        case NOTIF_WARN_ICP_CAPTURE_TIMEOUT:
            LOG_WARN("Pipeline: %u, NOTIF_WARN_ICP_CAPTURE_TIMEOUT\n", oNotificationData.uIndex);
            break;
        case NOTIF_ERROR_ICP_BAD_INPUT_STREAM:
            LOG_ERR("Pipeline: %u, NOTIF_ERROR_ICP_BAD_INPUT_STREAM\n", oNotificationData.uIndex);
            if (!bIgnoreError) {
                m_bInError = true; // Treat this as fatal error only if link recovery is not enabled.
            }
            break;
        case NOTIF_ERROR_ICP_CAPTURE_FAILURE:
            LOG_ERR("Pipeline: %u, NOTIF_ERROR_ICP_CAPTURE_FAILURE\n", oNotificationData.uIndex);
            m_bInError = true;
            break;
        case NOTIF_ERROR_ICP_EMB_DATA_PARSE_FAILURE:
            LOG_ERR("Pipeline: %u, NOTIF_ERROR_ICP_EMB_DATA_PARSE_FAILURE\n", oNotificationData.uIndex);
            m_bInError = true;
            break;
        case NOTIF_ERROR_ISP_PROCESSING_FAILURE:
            LOG_ERR("Pipeline: %u, NOTIF_ERROR_ISP_PROCESSING_FAILURE\n", oNotificationData.uIndex);
            m_bInError = true;
            break;
        case NOTIF_ERROR_ACP_PROCESSING_FAILURE:
            LOG_ERR("Pipeline: %u, NOTIF_ERROR_ACP_PROCESSING_FAILURE\n", oNotificationData.uIndex);
            m_bInError = true;
            break;
        case NOTIF_ERROR_CDI_SET_SENSOR_CTRL_FAILURE:
            LOG_ERR("Pipeline: %u, NOTIF_ERROR_CDI_SET_SENSOR_CTRL_FAILURE\n", oNotificationData.uIndex);
            if (!bIgnoreError) {
                m_bInError = true; // Treat this as fatal error only if link recovery is not enabled.
            }
            break;
        case NOTIF_ERROR_INTERNAL_FAILURE:
            LOG_ERR("Pipeline: %u, NOTIF_ERROR_INTERNAL_FAILURE\n", oNotificationData.uIndex);
            m_bInError = true;
            break;
        default:
            LOG_WARN("Pipeline: %u, Unknown/Invalid notification\n", oNotificationData.uIndex);
            break;
        }

        return;
    }

    static void EventQueueThreadFunc(CPipelineNotificationHandler *pThis)
    {
        SIPLStatus status = NVSIPL_STATUS_OK;
        NotificationData notificationData;

        if ((pThis == nullptr) || (pThis->m_pNotificationQueue == nullptr)) {
            LOG_ERR("Invalid thread data\n");
            return;
        }

        pthread_setname_np(pthread_self(), "PipelineEvent");

        while (!pThis->m_bQuit) {
            status = pThis->m_pNotificationQueue->Get(notificationData, EVENT_QUEUE_TIMEOUT_US);
            if (status == NVSIPL_STATUS_OK) {
                pThis->OnEvent(notificationData);
            } else if (status == NVSIPL_STATUS_TIMED_OUT) {
                LOG_DBG("Queue timeout\n");
            } else if (status == NVSIPL_STATUS_EOF) {
                LOG_DBG("Queue shutdown\n");
                pThis->m_bQuit = true;
            } else {
                LOG_ERR("Unexpected queue return status\n");
                pThis->m_bQuit = true;
            }
        }
    }

    uint32_t m_uNumFrameDrops = 0U;
    bool m_bInError = false;
    std::unique_ptr<std::thread> m_upThread = nullptr;
    INvSIPLNotificationQueue *m_pNotificationQueue = nullptr;
    bool m_bQuit = false;
};

class CPipelineFrameQueueHandler
{
public:
    //! Initializes the Pipeline Frame Queue Handler
    SIPLStatus Init(uint32_t uSensor, INvSIPLFrameCompletionQueue *pFrameCompletionQueue, CMaster *pMaster)
    {
        m_uSensor = uSensor;
        m_pMaster = pMaster;
        m_pFrameCompletionQueue = pFrameCompletionQueue;
        LOG_DBG("FrameQueueHandler, m_pMaster: %p, m_pFrameCompletionQueue: %p\n", m_pMaster, m_pFrameCompletionQueue);
        m_upThread.reset(new std::thread(FrameCompletionQueueThreadFunc, this));
        return NVSIPL_STATUS_OK;
    }

    void Deinit()
    {
        m_bQuit = true;
        if (m_upThread != nullptr) {
            m_upThread->join();
            m_upThread.reset();
        }
    }

    static void FrameCompletionQueueThreadFunc(CPipelineFrameQueueHandler *pThis)
    {
        SIPLStatus status = NVSIPL_STATUS_OK;
        INvSIPLClient::INvSIPLBuffer *pBuffer = nullptr;

        pthread_setname_np(pthread_self(), "FrameQueue");

        while (!pThis->m_bQuit) {
            status = pThis->m_pFrameCompletionQueue->Get(pBuffer, IMAGE_QUEUE_TIMEOUT_US);
            LOG_DBG("FrameCompletionQueueThreadFunc, status: %u\n", status);
            if (status == NVSIPL_STATUS_OK) {
                status = pThis->m_pMaster->OnFrameAvailable(pThis->m_uSensor, pBuffer);
                if (status != NVSIPL_STATUS_OK) {
                    LOG_ERR("OnFrameAvailable failed. (status:%u)\n", status);
                    pThis->m_bQuit = true;
                    pBuffer->Release();
                    return;
                }
                status = pBuffer->Release();
                if (status != NVSIPL_STATUS_OK) {
                    pThis->m_bQuit = true;
                    return;
                }

            } else if (status == NVSIPL_STATUS_TIMED_OUT) {
                LOG_WARN("CPipelineFrameQueueHandler Queue timeout m_uSensor=%u\n",pThis->m_uSensor);
            } else if (status == NVSIPL_STATUS_EOF) {
                LOG_DBG("CPipelineFrameQueueHandler Queue shutdown\n");
                pThis->m_bQuit = true;
                return;
            } else {
                LOG_ERR("Unexpected queue return status: %u\n", status);
                pThis->m_bQuit = true;
                return;
            }
        }
    }

    virtual ~CPipelineFrameQueueHandler()
    {
        Deinit();
    }

    uint32_t m_uSensor = -1U;
    CMaster* m_pMaster = nullptr;
    std::unique_ptr<std::thread> m_upThread = nullptr;
    INvSIPLFrameCompletionQueue *m_pFrameCompletionQueue = nullptr;
    bool m_bQuit = false;
};

static AppType GetAppType(CCmdLineParser &cmdline)
{
   if (!cmdline.bMultiProcess) {
       return SINGLE_PROCESS;
   } else if (cmdline.bIsProducer) {
       return IPC_SIPL_PRODUCER;
   } else if (cmdline.bIsConsumer && cmdline.sConsumerType == "cuda") {
       return IPC_CUDA_CONSUMER;
   } else {
       return IPC_ENC_CONSUMER;
   }
}

int main(int argc, char *argv[])
{
    pthread_setname_np(pthread_self(), "Main");

    bQuit = false;

    LOG_MSG("Checking SIPL version\n");
    NvSIPLVersion oVer;
    NvSIPLGetVersion(oVer);

    LOG_MSG("NvSIPL library version: %u.%u.%u\n", oVer.uMajor, oVer.uMinor, oVer.uPatch);
    LOG_MSG("NVSIPL header version: %u %u %u\n", NVSIPL_MAJOR_VER, NVSIPL_MINOR_VER, NVSIPL_PATCH_VER);
    if (oVer.uMajor != NVSIPL_MAJOR_VER || oVer.uMinor != NVSIPL_MINOR_VER || oVer.uPatch != NVSIPL_PATCH_VER) {
        LOG_ERR("NvSIPL library and header version mismatch\n");
    }

    LOG_INFO("Parsing command line arguments\n");
    CCmdLineParser cmdline;
    auto ret = cmdline.Parse(argc, argv);
    if (ret != 0) {
        // No need to print any error, Parse() would have printed error.
        return -1;
    }
    AppType appType = GetAppType(cmdline);
    bool producerResident = appType == SINGLE_PROCESS || appType == IPC_SIPL_PRODUCER;
    LOG_INFO("appType: %u, producerResident: %u\n", appType, producerResident);

    bIgnoreError = cmdline.bIgnoreError;
    // Set verbosity level
    LOG_INFO("Setting verbosity level: %u\n", cmdline.verbosity);
    INvSIPLQueryTrace::GetInstance()->SetLevel((INvSIPLQueryTrace::TraceLevel)cmdline.verbosity);
#if !NV_IS_SAFETY
    INvSIPLTrace::GetInstance()->SetLevel((INvSIPLTrace::TraceLevel)cmdline.verbosity);
#endif // !NV_IS_SAFETY
    CLogger::GetInstance().SetLogLevel((CLogger::LogLevel) cmdline.verbosity);

    LOG_MSG("Setting up signal handler\n");
    SigSetup();

    PlatformCfg pPlatformCfg;

    auto pQuery = nvsipl::INvSIPLQuery::GetInstance();
    if (pQuery == nullptr) 
    {
        LOG_ERR("INvSIPLQuery::GetInstance() failed\n");
    }
    auto ret1 = pQuery->ParseDatabase();
    if (ret1 != nvsipl::NVSIPL_STATUS_OK) 
    {
        LOG_ERR("INvSIPLQuery::ParseDatabase failed\n");
    }
    ret1 = pQuery->GetPlatformCfg(cmdline.sPlatformCfgName.c_str(), pPlatformCfg);
    if (ret1 != nvsipl::NVSIPL_STATUS_OK) 
    { 
        LOG_ERR("GetPlatformCfg failed. status: %d,pcn:%s\n", ret1, cmdline.sPlatformCfgName.c_str());
    }
    ret1 = pQuery->ApplyMask(pPlatformCfg, cmdline.vMasks);
    if (ret1 != nvsipl::NVSIPL_STATUS_OK) 
    {
        LOG_ERR("Failed to apply mask \n");
    }
    // PlatformCfg *pPlatformCfg = nullptr;
    // if (cmdline.sPlatformCfgName == "F008A120RM0A_CPHY_x4") {
    //     pPlatformCfg = &platformCfgAr0820;
    // } else if (cmdline.sPlatformCfgName == "SF3324_CPHY_x4") {
    //     pPlatformCfg = &platformCfgSf3324;
    // } else {
    //     LOG_ERR("Unexpected platform configuration\n");
    //     return NVSIPL_STATUS_ERROR;
    // }

    LOG_MSG("Creating camera master appType %u\n",appType);
    upMaster.reset(new CMaster(appType));
    CHK_PTR_AND_RETURN(upMaster, "Master creation");

    LOG_MSG("Setting up master appType %u\n",appType);
    auto status = upMaster->Setup(cmdline.bMultiProcess);
    CHK_STATUS_AND_RETURN(status, "Master setup");

    std::vector<CameraModuleInfo> vCameraModules;

    // for each sensor
    for (auto d = 0u; d != pPlatformCfg.numDeviceBlocks; d++) {
        auto db = pPlatformCfg.deviceBlockList[d];
        for (auto m = 0u; m != db.numCameraModules; m++) {
            vCameraModules.push_back(db.cameraModuleInfoList[m]);
        }
    }

    NvSIPLDeviceBlockQueues deviceBlockQueues;
    NvSIPLPipelineConfiguration pipelineCfg {};
    NvSIPLPipelineQueues pipelineQueues {};
    vector<std::unique_ptr<CPipelineFrameQueueHandler>> vupFrameCompletionQueueHandler;
    vector<std::unique_ptr<CPipelineNotificationHandler>> vupNotificationHandler;
    vector<std::unique_ptr<CDeviceBlockNotificationHandler>> vupDeviceBlockNotifyHandler;
    vector<unique_ptr<CProfiler>> vupProfilers;

    if (producerResident) {
        status = upMaster->SetPlatformConfig(&pPlatformCfg, deviceBlockQueues);
        CHK_STATUS_AND_RETURN(status, "Master SetPlatformConfig");

        pipelineCfg.captureOutputRequested = false;
        pipelineCfg.isp0OutputRequested = true;
        pipelineCfg.isp1OutputRequested = false;
        pipelineCfg.isp2OutputRequested = false;

        for (const auto& module : vCameraModules) {
            uint32_t uSensorId = module.sensorInfo.id;
            status = upMaster->SetPipelineConfig(uSensorId, pipelineCfg, pipelineQueues);
            CHK_STATUS_AND_RETURN(status, "Master SetPipelineConfig");

            //Register handlers
            auto upFrameCompletionQueueHandler = std::unique_ptr<CPipelineFrameQueueHandler>(new CPipelineFrameQueueHandler());
            CHK_PTR_AND_RETURN(upFrameCompletionQueueHandler, "Frame Completion Queue handler creation");

            upFrameCompletionQueueHandler->Init(uSensorId, pipelineQueues.isp0CompletionQueue, upMaster.get());
            CHK_STATUS_AND_RETURN(status, "Frame Completion Queues Handler Init");

            vupFrameCompletionQueueHandler.push_back(move(upFrameCompletionQueueHandler));

            auto upNotificationHandler = std::unique_ptr<CPipelineNotificationHandler>(new CPipelineNotificationHandler());
            CHK_PTR_AND_RETURN(upNotificationHandler, "Notification handler creation");

            status = upNotificationHandler->Init(uSensorId, pipelineQueues.notificationQueue);
            CHK_STATUS_AND_RETURN(status, "Notification Handler Init");

            vupNotificationHandler.push_back(move(upNotificationHandler));
        }

        LOG_MSG("Initializing master interface appType %u\n",appType);
        status = upMaster->InitPipeline();
        CHK_STATUS_AND_RETURN(status, "Init pipeline");

        for (auto d = 0u; d != pPlatformCfg.numDeviceBlocks; d++) {
            auto upDeviceBlockNotifyHandler = std::unique_ptr<CDeviceBlockNotificationHandler>(new CDeviceBlockNotificationHandler());
            CHK_PTR_AND_RETURN(upDeviceBlockNotifyHandler, "Device Block Notification handler creation");

            status = upDeviceBlockNotifyHandler->Init(d, pPlatformCfg.deviceBlockList[d], deviceBlockQueues.notificationQueue[d]);
            CHK_STATUS_AND_RETURN(status, "Device Block Notification Handler Init");

            vupDeviceBlockNotifyHandler.push_back(move(upDeviceBlockNotifyHandler));
        }
    }

    LOG_MSG("RegisterSource. appType %u\n",appType);
    for (auto& module : vCameraModules) {
        unique_ptr<CProfiler> upProfiler = unique_ptr<CProfiler>(new CProfiler());
        CHK_PTR_AND_RETURN(upProfiler, "Profiler creation");
        CProfiler *pProfiler = upProfiler.get();

        upProfiler->Init(module.sensorInfo.id, INvSIPLClient::ConsumerDesc::OutputType::ISP0);
        vupProfilers.push_back(move(upProfiler));

        status = upMaster->RegisterSource(&module.sensorInfo, pProfiler,cmdline.consumerId);
        CHK_STATUS_AND_RETURN(status, "Register source");
    }

    LOG_MSG("start upMaster->InitStream  appType %u\n",appType);
    status = upMaster->InitStream();
    CHK_STATUS_AND_RETURN(status, "Init stream");

    if (producerResident) {
        for (const auto& module : vCameraModules) {
            uint32_t uSensorId = module.sensorInfo.id;

            //load nito
            std::vector<uint8_t> blob;
            status = LoadNITOFile(cmdline.sNitoFolderPath, module.name, blob);
            CHK_STATUS_AND_RETURN(status, "Load NITO file");

            LOG_INFO("RegisterAutoControl.\n");
            status = upMaster->RegisterAutoControl(uSensorId, NV_PLUGIN, nullptr, blob);
            CHK_STATUS_AND_RETURN(status, "RegisterAutoControl");
        }
    }
    LOG_MSG("upMaster->StartStream() appType %u\n",appType);
    status = upMaster->StartStream();
    CHK_STATUS_AND_RETURN(status, "Start channel");

    if (producerResident) {
        LOG_INFO("upMaster->StartPipeline().\n");
        status = upMaster->StartPipeline();
        CHK_STATUS_AND_RETURN(status, "Start pipeline");
    }

    LOG_MSG("upMaster->StartStream() finished appType %u\n",appType);
    // Spawn a background thread to accept user's runtime command
    // std::thread([&]
    // {
    //     pthread_setname_np(pthread_self(), "RuntimeMenu");

    //     while (!bQuit) {
    //         cout << "Enter 'q' to quit the application\n";
    //         char line[256];
    //         cout << "-\n";
    //         cin.getline(line, 256);
    //         if (line[0] == 'q') {
    //             bQuit = true;
    //         }
    //     }
    // }).detach();

    uint64_t uFrameCountDelta = 0u;
    // Wait for quit
    while (!bQuit) {
        // Wait for SECONDS_PER_ITERATION
        auto oStartTime = chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::seconds(SECONDS_PER_ITERATION));
        auto uTimeElapsedMs = chrono::duration<double, std::milli> (chrono::steady_clock::now() - oStartTime).count();
        cout << "Output" << endl;

        for (auto &prof : vupProfilers) {
            prof->m_profData.profDataMut.lock();
            uFrameCountDelta = prof->m_profData.uFrameCount - prof->m_profData.uPrevFrameCount;
            prof->m_profData.uPrevFrameCount = prof->m_profData.uFrameCount;
            prof->m_profData.profDataMut.unlock();
            auto fps = uFrameCountDelta / (uTimeElapsedMs / 1000.0);
            string profName = "Sensor" + to_string(prof->m_uSensor) + "_Out"
                              + to_string(int(prof->m_outputType)) + "\t";
            cout << profName << "Frame rate (fps):\t\t" << fps << endl;
        }
        cout << endl;

        if (producerResident) {
            // Check for any asynchronous fatal errors reported by pipeline threads in the library
            for (auto &notificationHandler : vupNotificationHandler) {
                if (notificationHandler->IsPipelineInError()) {
                    bQuit = true;
                }
            }
            
            // Check for any asynchronous errors reported by the device blocks
            for (auto &notificationHandler : vupDeviceBlockNotifyHandler) {
                if (notificationHandler->IsDeviceBlockInError()) {
                    bQuit = true;
                }
            }
        }
    }

    bool bDeviceBlockError = false;
    bool bPipelineError = false;
    if (producerResident) {
        LOG_INFO("Stopping pipeline\n");
        status = upMaster->StopPipeline();
        CHK_STATUS_AND_RETURN(status, "Stop pipeline");

        for (auto &notificationHandler : vupDeviceBlockNotifyHandler) {
            LOG_INFO("Deinitializing devblk notificationHandler: %u\n", notificationHandler->m_uDevBlkIndex);
            bDeviceBlockError |= notificationHandler->IsDeviceBlockInError();
            notificationHandler->Deinit();
        }

        for (auto &notificationHandler : vupNotificationHandler) {
            LOG_INFO("Deinitializing pipeline notificationHandler: %u\n", notificationHandler->m_uSensor);
            bPipelineError |= notificationHandler->IsPipelineInError();
            notificationHandler->Deinit();
        }
        
        for (auto &frameCompletionQueueHandler : vupFrameCompletionQueueHandler) {
            LOG_INFO("Deinitializing frameCompletionQueueHandler: %u\n", frameCompletionQueueHandler->m_uSensor);
            frameCompletionQueueHandler->Deinit();
        }
    }
    
    if (upMaster != nullptr) {
        LOG_INFO("Stopping channels\n");
        upMaster->StopStream();
    }

    if (producerResident) {
        if (upMaster != nullptr) {
            LOG_DBG("De-initializing master\n");
            upMaster->DeinitPipeline();
        }
    }

    if (bPipelineError) {
        LOG_ERR("Pipeline failure\n");
        return -1;
    }

    if (bDeviceBlockError) {
        LOG_ERR("Device Block failure\n");
        return -1;
    }

    LOG_MSG("SUCCESS\n");
    return 0;
}
