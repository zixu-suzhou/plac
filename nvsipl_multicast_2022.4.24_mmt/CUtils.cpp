/*
 * Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "CUtils.hpp"
#include <cstring>
#include <sys/time.h>

using namespace std;

// Log utils
CLogger& CLogger::GetInstance()
{
    static CLogger instance;
    return instance;
}

void CLogger::SetLogLevel(LogLevel level)
{
    m_level = (level > LEVEL_DBG) ? LEVEL_DBG : level;
}

CLogger::LogLevel CLogger::GetLogLevel(void)
{
    return m_level;
}

void CLogger::SetLogStyle(LogStyle style)
{
    m_style = (style > LOG_STYLE_FUNCTION_LINE) ? LOG_STYLE_FUNCTION_LINE
                                                : style;
}

void CLogger::LogLevelMessageVa(LogLevel level, const char *functionName,
                                       uint32_t lineNumber, const char *prefix, const char *format,
                                                                    va_list ap)
{
    char str[256] = {'\0',};

    if (level > m_level) {
        return;
    }

    struct timeval tv;
    gettimeofday(&tv,nullptr);
    sprintf(str,"[%ld.%06ld]",tv.tv_sec,tv.tv_usec);

    strcpy(str, "nvsipl_multicast: ");
    switch (level) {
        case LEVEL_NONE:
            break;
        case LEVEL_ERR:
            strcat(str, "ERROR: ");
            break;
        case LEVEL_WARN:
            strcat(str, "WARNING: ");
            break;
        case LEVEL_INFO:
            break;
        case LEVEL_DBG:
            // Empty
            break;
    }

    if (strlen(prefix) != 0) {
        strcat(str, prefix);
    }

    vsnprintf(str + strlen(str), sizeof(str) - strlen(str), format, ap);

    if (m_style == LOG_STYLE_NORMAL) {
        if (strlen(str) != 0 && str[strlen(str) - 1] == '\n') {
            strcat(str, "\n");
        }
    } else if (m_style == LOG_STYLE_FUNCTION_LINE) {
        if (strlen(str) != 0 && str[strlen(str) - 1] == '\n') {
            str[strlen(str) - 1] = 0;
        }
        snprintf(str + strlen(str), sizeof(str) - strlen(str), " at %s():%d\n",
                                                     functionName, lineNumber);
    }

    cout << str;
}

void CLogger::LogLevelMessage(LogLevel level, const char *functionName,
                               uint32_t lineNumber, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    LogLevelMessageVa(level, functionName, lineNumber, "", format, ap);
    va_end(ap);
}

void CLogger::LogLevelMessage(LogLevel level, std::string functionName,
                               uint32_t lineNumber, std::string format, ...)
{
    va_list ap;
    va_start(ap, format);
    LogLevelMessageVa(level, functionName.c_str(), lineNumber, "",
                                                       format.c_str(), ap);
    va_end(ap);
}

void CLogger::PLogLevelMessage(LogLevel level, const char *functionName,
                               uint32_t lineNumber, std::string prefix, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    
    LogLevelMessageVa(level, functionName, lineNumber, prefix.c_str(), format, ap);
    va_end(ap);
}

void CLogger::PLogLevelMessage(LogLevel level, std::string functionName,
                               uint32_t lineNumber, std::string prefix, std::string format, ...)
{
    va_list ap;
    va_start(ap, format);
    LogLevelMessageVa(level, functionName.c_str(), lineNumber, prefix.c_str(),
                                                       format.c_str(), ap);
    va_end(ap);
}

void CLogger::LogMessageVa(const char *format, va_list ap)
{
    char str[128] = {'\0',};
    struct timeval tv;
    gettimeofday(&tv,nullptr);
    int n = sprintf(str,"[%ld.%06ld]",tv.tv_sec,tv.tv_usec);

    vsnprintf(str+n, sizeof(str) - n, format, ap);
    cout << str;
}

void CLogger::LogMessage(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    LogMessageVa(format, ap);
    va_end(ap);
}

void CLogger::LogMessage(std::string format, ...)
{
    va_list ap;
    va_start(ap, format);
    LogMessageVa(format.c_str(), ap);
    va_end(ap);
}

/* Loads NITO file for given camera module.
 The function assumes the .nito files to be named same as camera module name.
 */
SIPLStatus LoadNITOFile(std::string folderPath,
                        std::string moduleName,
                        std::vector<uint8_t>& nito)
{
    // Set up blob file
    string nitoFilePath = (folderPath != "") ? folderPath : "/opt/nvidia/nvmedia/nit/";
    string nitoFile = nitoFilePath + moduleName + ".nito";

    string moduleNameLower{};
    for (auto& c : moduleName) {
        moduleNameLower.push_back(std::tolower(c));
    }
    string nitoFileLower = nitoFilePath + moduleNameLower + ".nito";
    string nitoFileDefault = nitoFilePath + "default.nito";

    // Open NITO file
    auto fp = fopen(nitoFile.c_str(), "rb");
    if (fp == nullptr) {
        LOG_INFO("File \"%s\" not found\n", nitoFile.c_str());
        // Try lower case name
        fp = fopen(nitoFileLower.c_str(), "rb");
        if (fp == nullptr) {
            LOG_INFO("File \"%s\" not found\n", nitoFileLower.c_str());
            LOG_ERR("Unable to open NITO file for module \"%s\", image quality is not supported!\n", moduleName.c_str());
            return NVSIPL_STATUS_BAD_ARGUMENT;
        } else {
            LOG_MSG("nvsipl_multicast: Opened NITO file for module \"%s\"\n", moduleName.c_str());
        }
    } else {
        LOG_MSG("nvsipl_multicast: Opened NITO file for module \"%s\"\n", moduleName.c_str());
    }

    // Check file size
    fseek(fp, 0, SEEK_END);
    auto fsize = ftell(fp);
    rewind(fp);

    if (fsize <= 0U) {
        LOG_ERR("NITO file for module \"%s\" is of invalid size\n", moduleName.c_str());
        fclose(fp);
        return NVSIPL_STATUS_BAD_ARGUMENT;
    }

    /* allocate blob memory */
    nito.resize(fsize);

    /* load nito */
    auto result = (long int) fread(nito.data(), 1, fsize, fp);
    if (result != fsize) {
        LOG_ERR("Fail to read data from NITO file for module \"%s\", image quality is not supported!\n", moduleName.c_str());
        nito.resize(0);
        fclose(fp);
        return NVSIPL_STATUS_BAD_ARGUMENT;
    }
    /* close file */
    fclose(fp);

    LOG_INFO("data from NITO file loaded for module \"%s\"\n", moduleName.c_str());

    return NVSIPL_STATUS_OK;
}

SIPLStatus GetConsumerTypeFromAppType(AppType appType, ConsumerType& consumerType)
{
    SIPLStatus status = NVSIPL_STATUS_OK;

    switch (appType) {
        case IPC_CUDA_CONSUMER:
            consumerType = CUDA_CONSUMER;
            break;
        case IPC_ENC_CONSUMER:
            consumerType = ENC_CONSUMER;
            break;
        default:
            status = NVSIPL_STATUS_BAD_ARGUMENT;
            break;
    }

    return status;
}
