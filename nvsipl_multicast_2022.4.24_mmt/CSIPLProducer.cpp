// Copyright (c) 2022 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software, related documentation and any
// modifications thereto. Any use, reproduction, disclosure or distribution
// of this software and related documentation without an express license
// agreement from NVIDIA Corporation is strictly prohibited.

#include "CSIPLProducer.hpp"
#include "CPoolManager.hpp"

CSIPLProducer::CSIPLProducer(NvSciStreamBlock handle, uint32_t uSensor, INvSIPLCamera* pCamera) :
    CProducer("CSIPLProducer", handle, uSensor)
{
    m_pCamera = pCamera;

    for (uint32_t i = 0; i < MAX_PACKETS; i++) {
        m_rawBufObjs[i] = nullptr;
        m_ispBufObjs[i] = nullptr;
    }
    m_ispOutputType = INvSIPLClient::ConsumerDesc::OutputType::ISP0;
}

CSIPLProducer::~CSIPLProducer(void)
{
    PLOG_DBG("Release.\n");
    for (uint32_t i = 0; i < MAX_PACKETS; i++) {
        if (m_rawBufObjs[i] != nullptr) {
            NvSciBufObjFree(m_rawBufObjs[i]);
            m_rawBufObjs[i] = nullptr;
        }
        if (m_ispBufObjs[i] != nullptr) {
            NvSciBufObjFree(m_ispBufObjs[i]);
            m_ispBufObjs[i] = nullptr;
        }
    }

    if (m_rawBufAttrList != nullptr) {
        NvSciBufAttrListFree(m_rawBufAttrList);
        m_rawBufAttrList = nullptr;
    }

    DeleteImageGroups(m_imageGroupList);
    DeleteImages(m_imageList);
    std::vector<NvMediaImageGroup*>().swap(m_imageGroupList);
    std::vector<NvMediaImage*>().swap(m_imageList);
}

SIPLStatus CSIPLProducer::HandleClientInit(void)
{
    m_upDevice.reset(NvMediaDeviceCreate());
    if (m_upDevice == nullptr) {
        PLOG_ERR("NvMediaDeviceCreate failed\n");
        return NVSIPL_STATUS_OUT_OF_MEMORY;
    }

    return NVSIPL_STATUS_OK;
}

void CSIPLProducer::DeleteImageGroups(std::vector<NvMediaImageGroup*>& imageGroups)
{
    auto bufferPoolSize = imageGroups.size();
    for (auto i = 0u; i < bufferPoolSize; i++) {
        if (imageGroups[i] == nullptr) {
            PLOG_ERR("Attempt to destroy null image group\n");
            continue;
        }
        for (auto p = 0u; p < imageGroups[i]->numImages; p++) {
            if (imageGroups[i]->imageList[p] == nullptr) {
                PLOG_ERR("Attempt to destroy null image\n");
                continue;
            }
            NvMediaImageDestroy(imageGroups[i]->imageList[p]);
        }
        delete imageGroups[i];
    }
}

void CSIPLProducer::DeleteImages(std::vector<NvMediaImage*>& images)
{
    auto bufferPoolSize = images.size();
    for (auto i = 0u; i < bufferPoolSize; i++) {
       if (images[i] == nullptr) {
           PLOG_ERR("Attempt to destroy null image\n");
           continue;
       }
       NvMediaImageDestroy(images[i]);
    }
}

// Create and set CPU signaler and waiter attribute lists.
SIPLStatus CSIPLProducer::SetSyncAttrList(void)
{
    auto status = m_pCamera->FillNvSciSyncAttrList(m_uSensorId,
                                                   m_ispOutputType,
                                                   m_signalerAttrList,
                                                   NVMEDIA_SIGNALER);
    PCHK_STATUS_AND_RETURN(status, "Signaler INvSIPLCamera::FillNvSciSyncAttrList");

// Temporarily comment out the below code snippet to WAR the issue of
// failing to register sync object with ISP.
/*
    status = m_pCamera->FillNvSciSyncAttrList(m_uSensorId,
                                              m_ispOutputType,
                                              m_waiterAttrList,
                                              NVMEDIA_WAITER);
    PCHK_STATUS_AND_RETURN(status, "Waiter INvSIPLCamera::FillNvSciSyncAttrList");
*/
    NvSciSyncAttrKeyValuePair keyValue[2];
    bool cpuWaiter = true;
    keyValue[0].attrKey = NvSciSyncAttrKey_NeedCpuAccess;
    keyValue[0].value = (void *)&cpuWaiter;
    keyValue[0].len = sizeof(cpuWaiter);
    NvSciSyncAccessPerm cpuPerm = NvSciSyncAccessPerm_WaitOnly;
    keyValue[1].attrKey = NvSciSyncAttrKey_RequiredPerm;
    keyValue[1].value = (void*)&cpuPerm;
    keyValue[1].len = sizeof(cpuPerm);
    auto sciErr = NvSciSyncAttrListSetAttrs(m_waiterAttrList, keyValue, 2);
    PCHK_NVSCISTATUS_AND_RETURN(sciErr, "CPU waiter NvSciSyncAttrListSetAttrs");

    return NVSIPL_STATUS_OK;
}

SIPLStatus CSIPLProducer::CreateBufAttrLists(NvSciBufModule bufModule)
{
    //Create ISP buf attrlist
    auto status = CClientCommon::CreateBufAttrLists(bufModule);
    CHK_STATUS_AND_RETURN(status, "CClientCommon::CreateBufAttrList");

    // create raw buf attrlist
    auto sciErr = NvSciBufAttrListCreate(bufModule, &m_rawBufAttrList);
    PCHK_NVSCISTATUS_AND_RETURN(sciErr, "NvSciBufAttrListCreate");

    return NVSIPL_STATUS_OK;
}

SIPLStatus CSIPLProducer::SetDataBufAttrList(void)
{
    auto status = SetBufAttrList(INvSIPLClient::ConsumerDesc::OutputType::ICP, m_rawBufAttrList);
    PCHK_STATUS_AND_RETURN(status, "SetBufAttrList for RAW");

    status = SetBufAttrList(m_ispOutputType, m_bufAttrLists[DATA_ELEMENT_INDEX]);
    PCHK_STATUS_AND_RETURN(status, "SetBufAttrList for ISP");

    return NVSIPL_STATUS_OK;
}

// Buffer setup functions
SIPLStatus CSIPLProducer::SetBufAttrList(
    INvSIPLClient::ConsumerDesc::OutputType outputType,
    NvSciBufAttrList bufAttrList)
{
    NvSIPLImageAttr attr;

    auto status = m_pCamera->GetImageAttributes(m_uSensorId, outputType, attr);
    PCHK_STATUS_AND_RETURN(status, "GetImageAttributes");

    if (outputType == INvSIPLClient::ConsumerDesc::OutputType::ISP0) {
        // Add CPU_ACCESS_CACHED attribute if not already set, to be backward compatible
        // TODO: Remove this, instead get the attributes from consumers and do the attribute reconciliation
        bool found = false;
        for (const auto &it : attr.surfaceAllocAttr) {
            if (it.type == NVM_SURF_ATTR_CPU_ACCESS) {
                found = true;
                break;
            }
        }
        if (!found) {
            attr.surfaceAllocAttr.push_back({NVM_SURF_ATTR_CPU_ACCESS, NVM_SURF_ATTR_CPU_ACCESS_CACHED});
        }
    }

    NvSciBufAttrValAccessPerm access_perm = NvSciBufAccessPerm_ReadWrite;
    NvSciBufAttrKeyValuePair attrKvp = {NvSciBufGeneralAttrKey_RequiredPerm,
                                        &access_perm,
                                        sizeof(access_perm)};
    auto sciErr = NvSciBufAttrListSetAttrs(bufAttrList, &attrKvp, 1);
    PCHK_NVSCISTATUS_AND_RETURN(sciErr, "NvSciBufAttrListSetAttrs");

    auto nvmStatus = NvMediaImageFillNvSciBufAttrs(m_upDevice.get(),
                                                   attr.surfaceType,
                                                   attr.surfaceAllocAttr.data(),
                                                   attr.surfaceAllocAttr.size(),
                                                   0,
                                                   bufAttrList);
    PCHK_NVMSTATUS_AND_RETURN(nvmStatus, "NvMediaImageFillNvSciBufAttrs");

    return NVSIPL_STATUS_OK;
}

// Create client buffer objects from NvSciBufObj
SIPLStatus CSIPLProducer::MapDataBuffer(uint32_t packetIndex)
{
    PLOG_DBG("Mapping data buffer, packetIndex: %u.\n", packetIndex);
    auto sciErr = NvSciBufObjDup(m_packets[packetIndex].dataObj, &m_ispBufObjs[packetIndex]);
    PCHK_NVSCISTATUS_AND_RETURN(sciErr, "NvSciBufObjDup");

    return NVSIPL_STATUS_OK;
}

// Create client buffer objects from NvSciBufObj
SIPLStatus CSIPLProducer::MapMetaBuffer(uint32_t packetIndex)
{
    PLOG_DBG("Mapping meta buffer, packetIndex: %u.\n", packetIndex);
    auto sciErr = NvSciBufObjGetCpuPtr(m_packets[packetIndex].metaObj, (void**)&m_metaPtrs[packetIndex]);
    PCHK_NVSCISTATUS_AND_RETURN(sciErr, "NvSciBufObjGetCpuPtr");

    return NVSIPL_STATUS_OK;
}

SIPLStatus CSIPLProducer::RegisterSignalSyncObj(void)
{
    //Only one signalSyncObj
    auto status = m_pCamera->RegisterNvSciSyncObj(m_uSensorId, m_ispOutputType, NVMEDIA_EOFSYNCOBJ, m_signalSyncObj);
    PCHK_STATUS_AND_RETURN(status, "INvSIPLCamera::RegisterNvSciSyncObj");

    return NVSIPL_STATUS_OK;
}

SIPLStatus CSIPLProducer::RegisterWaiterSyncObj(uint32_t index)
{
// Temporarily comment out the below code snippet to WAR the issue of
// failing to register sync object with ISP.
/*
    auto status = m_pCamera->RegisterNvSciSyncObj(m_uSensorId,
                                                  m_ispOutputType,
                                                  NVMEDIA_PRESYNCOBJ,
                                                  m_waiterSyncObjs[index]);
    PCHK_STATUS_AND_RETURN(status, "INvSIPLCamera::RegisterNvSciSyncObj");
*/

    return NVSIPL_STATUS_OK;
}

SIPLStatus CSIPLProducer::HandleSetupComplete(void)
{
    auto status = CProducer::HandleSetupComplete();
    PCHK_STATUS_AND_RETURN(status, "HandleSetupComplete");

    //Alloc raw buffers
    status = CPoolManager::ReconcileAndAllocBuffers(m_rawBufAttrList, &m_rawBufObjs[0]);
    PCHK_STATUS_AND_RETURN(status, "CPoolManager::ReconcileAndAllocBuffers");

    status = RegisterBuffers();
    PCHK_STATUS_AND_RETURN(status, "RegisterBuffers");

    return NVSIPL_STATUS_OK;
}

SIPLStatus CSIPLProducer::RegisterBuffers(void)
{
    PLOG_DBG("RegisterBuffers\n");
    m_imageGroupList.resize(MAX_PACKETS);
    for(auto i = 0u; i < MAX_PACKETS; i++) {
        auto imgGrp =  new (std::nothrow) NvMediaImageGroup;
        PCHK_PTR_AND_RETURN(imgGrp, "new NvMediaImageGroup");
        imgGrp->numImages = 1;
        auto nvmStatus = NvMediaImageCreateFromNvSciBuf(m_upDevice.get(), m_rawBufObjs[i], &imgGrp->imageList[0]);
        PCHK_NVMSTATUS_AND_RETURN(nvmStatus, "NvMediaImageCreateFromNvSciBuf");
        nvmStatus = NvMediaImageSetTag(imgGrp->imageList[0], (void *)(&m_packets[i].cookie));
        PCHK_NVMSTATUS_AND_RETURN(nvmStatus, "NvMediaImageSetTag");
        m_imageGroupList[i] = imgGrp;
    }
    auto status = m_pCamera->RegisterImageGroups(m_uSensorId, m_imageGroupList);
    PCHK_STATUS_AND_RETURN(status, "INvSIPLCamera::RegisterImageGroups");

    m_imageList.resize(MAX_PACKETS);
    for(auto i = 0u; i < MAX_PACKETS; i++) {
        auto nvmStatus = NvMediaImageCreateFromNvSciBuf(m_upDevice.get(), m_ispBufObjs[i], &m_imageList[i]);
        PCHK_NVMSTATUS_AND_RETURN(nvmStatus, "NvMediaImageCreateFromNvSciBuf");
        nvmStatus = NvMediaImageSetTag(m_imageList[i], (void *)(&m_packets[i].cookie));
        PCHK_NVMSTATUS_AND_RETURN(nvmStatus, "NvMediaImageSetTag");
    }
    status = m_pCamera->RegisterImages(m_uSensorId, m_ispOutputType, m_imageList);
    PCHK_STATUS_AND_RETURN(status, "INvSIPLCamera::RegisterImages");

    return NVSIPL_STATUS_OK;
}

//Before calling PreSync, m_nvmBuffers[packetIndex] should already be filled.
SIPLStatus CSIPLProducer::InsertPrefence(uint32_t packetIndex, NvSciSyncFence &prefence)
{
// Temporarily comment out the below code snippet to WAR the issue of
// failing to register sync object with ISP.
/*
    PLOG_DBG("AddPrefence, packetIndex: %u\n", packetIndex);
    auto status = m_nvmBuffers[packetIndex]->AddNvSciSyncPrefence(prefence);
    PCHK_STATUS_AND_RETURN(status, "AddNvSciSyncPrefence");
*/

    return NVSIPL_STATUS_OK;
}

SIPLStatus CSIPLProducer::GetPostfence(uint32_t packetIndex, NvSciSyncFence *pPostfence)
{
    auto status = m_nvmBuffers[packetIndex]->GetEOFNvSciSyncFence(pPostfence);
    PCHK_STATUS_AND_RETURN(status, "GetEOFNvSciSyncFence");

    return NVSIPL_STATUS_OK;
}

void CSIPLProducer::OnPacketGotten(uint32_t packetIndex)
{
    m_nvmBuffers[packetIndex]->Release();
}

SIPLStatus CSIPLProducer::MapPayload(void *pBuffer, uint32_t& packetIndex)
{
    NvMediaImage *pImage = nullptr;
    void *imageTag = nullptr;
    NvSciStreamCookie cookie = -1U;

    INvSIPLClient::INvSIPLNvMBuffer* pNvMBuf = reinterpret_cast<INvSIPLClient::INvSIPLNvMBuffer*>(pBuffer);
    pImage = pNvMBuf->GetImage();
    PCHK_PTR_AND_RETURN(pImage, "INvSIPLClient::INvSIPLNvMBuffer::GetImage");
    const NvMediaStatus nvmStatus = NvMediaImageGetTag(pImage, &imageTag);
    PCHK_NVMSTATUS_AND_RETURN(nvmStatus, "NvMediaImageGetTag");
    PCHK_PTR_AND_RETURN(imageTag, "NvMediaImageGetTag");
    cookie = *((NvSciStreamCookie *)imageTag);
    auto status = GetIndexFromCookie(cookie, packetIndex);
    PCHK_STATUS_AND_RETURN(status, "GetIndexFromCookie");
    PLOG_DBG("MapPayload, packetIndex: %u\n", packetIndex);
    if (m_metaPtrs[packetIndex] != nullptr) {
        const INvSIPLClient::ImageMetaData &md = pNvMBuf->GetImageData();
        m_metaPtrs[packetIndex]->frameCaptureTSC = md.frameCaptureTSC;
    }
    m_nvmBuffers[packetIndex] = pNvMBuf;
    m_nvmBuffers[packetIndex]->AddRef();

    return NVSIPL_STATUS_OK;
}
