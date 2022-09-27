// Copyright (c) 2022 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software, related documentation and any
// modifications thereto. Any use, reproduction, disclosure or distribution
// of this software and related documentation without an express license
// agreement from NVIDIA Corporation is strictly prohibited.

#ifndef CSIPLPRODUCER_HPP
#define CSIPLPRODUCER_HPP

#include "CProducer.hpp"

// nvmedia includes
#include "nvmedia_core.h"
#include "nvmedia_common.h"
#include "nvmedia_image.h"
#include "nvmedia_image_nvscibuf.h"
#include "NvSIPLCamera.hpp"

class CSIPLProducer: public CProducer
{
public:
    CSIPLProducer() = delete;
    CSIPLProducer(
        NvSciStreamBlock handle,
        uint32_t uSensor,
        INvSIPLCamera* pCamera);
    virtual ~CSIPLProducer(void);

    SIPLStatus Post(INvSIPLClient::INvSIPLNvMBuffer *pBuffer);
protected:
    virtual SIPLStatus HandleClientInit(void) override;
    virtual SIPLStatus CreateBufAttrLists(NvSciBufModule bufModule) override;
    virtual SIPLStatus SetDataBufAttrList(void) override;
    virtual SIPLStatus SetSyncAttrList(void) override;
    virtual void OnPacketGotten(uint32_t packetIndex) override;
    virtual SIPLStatus RegisterSignalSyncObj(void) override;
    virtual SIPLStatus RegisterWaiterSyncObj(uint32_t index) override;
    virtual SIPLStatus HandleSetupComplete(void) override;
    virtual SIPLStatus MapDataBuffer(uint32_t packetIndex) override;
    virtual SIPLStatus MapMetaBuffer(uint32_t packetIndex) override;
    virtual SIPLStatus InsertPrefence(uint32_t packetIndex, NvSciSyncFence &prefence) override;
    virtual SIPLStatus GetPostfence(uint32_t packetIndex, NvSciSyncFence *pPostfence) override;
    virtual SIPLStatus MapPayload(void *pBuffer, uint32_t& packetIndex) override;
    virtual bool HasCpuWait(void) {return true;};

private:
    SIPLStatus RegisterBuffers(void);
    void DeleteImageGroups(std::vector<NvMediaImageGroup*>& imageGroups);
    void DeleteImages(std::vector<NvMediaImage*>& images);
    SIPLStatus SetBufAttrList(INvSIPLClient::ConsumerDesc::OutputType outputType,
        NvSciBufAttrList bufAttrList);

    INvSIPLCamera* m_pCamera;
    INvSIPLClient::ConsumerDesc::OutputType m_ispOutputType;
    std::unique_ptr<NvMediaDevice, CloseNvMediaDevice> m_upDevice;
    NvSciBufAttrList m_rawBufAttrList = nullptr;
    NvSciBufObj m_ispBufObjs[MAX_PACKETS];
    NvSciBufObj m_rawBufObjs[MAX_PACKETS];
    INvSIPLClient::INvSIPLNvMBuffer *m_nvmBuffers[MAX_PACKETS] {nullptr};
    std::vector<NvMediaImageGroup*> m_imageGroupList; // one per packet (ICP only)
    std::vector<NvMediaImage*> m_imageList;           // one per packet
    /* Virtual address for the meta buffer */
    MetaData* m_metaPtrs[MAX_PACKETS];
};
#endif
