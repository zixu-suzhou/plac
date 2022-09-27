/*
 * Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "nvmedia_image.h"
#include "nvmedia_core.h"
#include "nvscierror.h"

#include <iostream>
#include <memory>
#include <cstdarg>
#include <vector>

#include "NvSIPLCommon.hpp"

using namespace nvsipl;
using namespace std;

#ifndef CUTILS_HPP
#define CUTILS_HPP

/** Helper MACROS */
#define CHK_PTR_AND_RETURN(ptr, api) \
    if ((ptr) == nullptr) { \
        LOG_ERR("%s failed\n", (api)); \
        return NVSIPL_STATUS_OUT_OF_MEMORY; \
    }

#define CHK_STATUS_AND_RETURN(status, api) \
    if ((status) != NVSIPL_STATUS_OK) { \
        LOG_ERR("%s failed, status: %u\n", (api), (status)); \
        return (status); \
    }

#define CHK_NVMSTATUS_AND_RETURN(nvmStatus, api) \
    if ((nvmStatus) != NVMEDIA_STATUS_OK) { \
        LOG_ERR("%s failed, status: %u\n", (api), (nvmStatus)); \
        return NVSIPL_STATUS_ERROR; \
    }

#define CHK_NVSCISTATUS_AND_RETURN(nvSciStatus, api) \
    if ((nvSciStatus) != NvSciError_Success) { \
        LOG_ERR("%s failed, status: %u\n", (api), (nvSciStatus)); \
        return NVSIPL_STATUS_ERROR; \
    }

/* prefix help MACROS */
#define PCHK_PTR_AND_RETURN(ptr, api) \
    if ((ptr) == nullptr) { \
        PLOG_ERR("%s failed\n", (api)); \
        return NVSIPL_STATUS_OUT_OF_MEMORY; \
    }

#define PCHK_STATUS_AND_RETURN(status, api) \
    if ((status) != NVSIPL_STATUS_OK) { \
        PLOG_ERR("%s failed, status: %u\n", (api), (status)); \
        return (status); \
    }

#define PCHK_NVMSTATUS_AND_RETURN(nvmStatus, api) \
    if ((nvmStatus) != NVMEDIA_STATUS_OK) { \
        PLOG_ERR("%s failed, status: %u\n", (api), (nvmStatus)); \
        return NVSIPL_STATUS_ERROR; \
    }

#define PCHK_NVSCISTATUS_AND_RETURN(nvSciStatus, api) \
    if ((nvSciStatus) != NvSciError_Success) { \
        PLOG_ERR("%s failed, status: %u\n", (api), (nvSciStatus)); \
        return NVSIPL_STATUS_ERROR; \
    }

#define CHK_CUDASTATUS_AND_RETURN(cudaStatus, api) \
    if ((cudaStatus) != cudaSuccess) { \
        cout << api << " failed. " << cudaStatus << "(" << cudaGetErrorName(cudaStatus) << ")" << endl; \
        return NVSIPL_STATUS_ERROR; \
    }

#define CHK_CUDAERR_AND_RETURN(e, api)                          \
    {                                                           \
        auto ret = (e);                                         \
        if (ret != CUDA_SUCCESS)                                \
        {                                                       \
            cout << api << " CUDA error: " << std::hex << ret << endl; \
            return NVSIPL_STATUS_ERROR;                         \
        }                                                       \
    }

#define PCHK_NVSCICONNECT_AND_RETURN(nvSciStatus, event, api) \
    if (NvSciError_Success != nvSciStatus) { \
       cout << m_name << ": " << api << " connect failed. " << nvSciStatus << endl; \
       return NVSIPL_STATUS_ERROR; \
    } \
    if (event != NvSciStreamEventType_Connected) { \
        cout << m_name << ": " << api << " didn't receive connected event. " << endl; \
        return NVSIPL_STATUS_ERROR; \
    }

#define LINE_INFO __FUNCTION__, __LINE__

//! Quick-log a message at debugging level
#define LOG_DBG(...) \
    CLogger::GetInstance().LogLevelMessage(LEVEL_DBG, LINE_INFO, __VA_ARGS__)

#define PLOG_DBG(...) \
    CLogger::GetInstance().PLogLevelMessage(LEVEL_DBG, LINE_INFO, m_name + ": ", __VA_ARGS__)

//! Quick-log a message at info level
#define LOG_INFO(...) \
    CLogger::GetInstance().LogLevelMessage(LEVEL_INFO, LINE_INFO, __VA_ARGS__)

#define PLOG_INFO(...) \
    CLogger::GetInstance().PLogLevelMessage(LEVEL_INFO, LINE_INFO, m_name + ": ", __VA_ARGS__)

//! Quick-log a message at warning level
#define LOG_WARN(...) \
    CLogger::GetInstance().LogLevelMessage(LEVEL_WARN, LINE_INFO, __VA_ARGS__)

#define PLOG_WARN(...) \
    CLogger::GetInstance().PLogLevelMessage(LEVEL_WARN, LINE_INFO, m_name + ": ", __VA_ARGS__)

//! Quick-log a message at error level
#define LOG_ERR(...) \
    CLogger::GetInstance().LogLevelMessage(LEVEL_ERR, LINE_INFO, __VA_ARGS__)

#define PLOG_ERR(...) \
    CLogger::GetInstance().PLogLevelMessage(LEVEL_ERR, LINE_INFO, m_name + ": ", __VA_ARGS__)

//! Quick-log a message at preset level
#define LOG_MSG(...) \
    CLogger::GetInstance().LogMessage(__VA_ARGS__)

#define LEVEL_NONE CLogger::LogLevel::LEVEL_NO_LOG

#define LEVEL_ERR CLogger::LogLevel::LEVEL_ERROR

#define LEVEL_WARN CLogger::LogLevel::LEVEL_WARNING

#define LEVEL_INFO CLogger::LogLevel::LEVEL_INFORMATION

#define LEVEL_DBG CLogger::LogLevel::LEVEL_DEBUG

//! \brief Logger utility class
//! This is a singleton class - at most one instance can exist at all times.
class CLogger
{
public:
    //! enum describing the different levels for logging
    enum LogLevel
    {
        /** no log */
        LEVEL_NO_LOG = 0,
        /** error level */
        LEVEL_ERROR,
        /** warning level */
        LEVEL_WARNING,
        /** info level */
        LEVEL_INFORMATION,
        /** debug level */
        LEVEL_DEBUG
    };

    //! enum describing the different styles for logging
    enum LogStyle
    {
        LOG_STYLE_NORMAL = 0,
        LOG_STYLE_FUNCTION_LINE = 1
    };

    //! Get the logging instance.
    //! \return Reference to the Logger object.
    static CLogger& GetInstance();

    //! Set the level for logging.
    //! \param[in] eLevel The logging level.
    void SetLogLevel(LogLevel eLevel);

    //! Get the level for logging.
    LogLevel GetLogLevel(void);

    //! Set the style for logging.
    //! \param[in] eStyle The logging style.
    void SetLogStyle(LogStyle eStyle);

    //! Log a message (cstring).
    //! \param[in] eLevel The logging level,
    //! \param[in] pszunctionName Name of the function as a cstring.
    //! \param[in] sLineNumber Line number,
    //! \param[in] pszFormat Format string as a cstring.
    void LogLevelMessage(LogLevel eLevel,
                         const char *pszFunctionName,
                         uint32_t sLineNumber,
                         const char *pszFormat,
                         ...);

    //! Log a message (C++ string).
    //! \param[in] eLevel The logging level,
    //! \param[in] sFunctionName Name of the function as a C++ string.
    //! \param[in] sLineNumber Line number,
    //! \param[in] sFormat Format string as a C++ string.
    void LogLevelMessage(LogLevel eLevel,
                         std::string sFunctionName,
                         uint32_t sLineNumber,
                         std::string sFormat,
                         ...);

    //! Log a message (cstring).
    //! \param[in] eLevel The logging level,
    //! \param[in] pszunctionName Name of the function as a cstring.
    //! \param[in] sLineNumber Line number,
    //! \param[in] prefix Prefix string.
    //! \param[in] pszFormat Format string as a cstring.
    void PLogLevelMessage(LogLevel eLevel,
                         const char *pszFunctionName,
                         uint32_t sLineNumber,
                         std::string prefix,
                         const char *pszFormat,
                         ...);

    //! Log a message (C++ string).
    //! \param[in] eLevel The logging level,
    //! \param[in] sFunctionName Name of the function as a C++ string.
    //! \param[in] sLineNumber Line number,
    //! \param[in] prefix Prefix string.
    //! \param[in] sFormat Format string as a C++ string.
    void PLogLevelMessage(LogLevel eLevel,
                         std::string sFunctionName,
                         uint32_t sLineNumber,
                         std::string prefix,
                         std::string sFormat,
                         ...);

    //! Log a message (cstring) at preset level.
    //! \param[in] pszFormat Format string as a cstring.
    void LogMessage(const char *pszFormat,
                    ...);

    //! Log a message (C++ string) at preset level.
    //! \param[in] sFormat Format string as a C++ string.
    void LogMessage(std::string sFormat,
                    ...);

private:
    //! Need private constructor because this is a singleton.
    CLogger() = default;
    LogLevel m_level = LEVEL_ERR;
    LogStyle m_style = LOG_STYLE_NORMAL;

    void LogLevelMessageVa(LogLevel eLevel,
                           const char *pszFunctionName,
                           uint32_t sLineNumber,
                           const char *prefix,
                           const char *pszFormat,
                           va_list ap);
    void LogMessageVa(const char *pszFormat,
                      va_list ap);
};
// CLogger class

struct CloseNvMediaDevice {
    void operator ()(NvMediaDevice *device) const {
        NvMediaDeviceDestroy(device);
    }
};

SIPLStatus LoadNITOFile(std::string folderPath,
                        std::string moduleName,
                        std::vector<uint8_t>& nito);

enum AppType
{
    SINGLE_PROCESS = 0,
    IPC_SIPL_PRODUCER,
    IPC_CUDA_CONSUMER,
    IPC_ENC_CONSUMER
};

enum ConsumerType
{
    CUDA_CONSUMER = 0,
    ENC_CONSUMER
};

SIPLStatus GetConsumerTypeFromAppType(AppType appType, ConsumerType& consumerType);

#endif
