// Copyright (c) 2022 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software, related documentation and any
// modifications thereto. Any use, reproduction, disclosure or distribution
// of this software and related documentation without an express license
// agreement from NVIDIA Corporation is strictly prohibited.

#ifndef CENCCONSUMER_H
#define CENCCONSUMER_H

#include "CConsumer.hpp"
#include "NvSIPLClient.hpp"
#include "nvmedia_iep.h"
#include "NvSIPLDeviceBlockInfo.hpp"

class CEncConsumer: public CConsumer
{
    public:
        CEncConsumer() = delete;
        CEncConsumer(NvSciStreamBlock handle,
                          uint32_t uSensor,
                          NvSciStreamBlock queueHandle,
                          uint16_t encodeWidth,
                          uint16_t encodeHeight);
        virtual ~CEncConsumer(void);

    protected:
        virtual SIPLStatus HandleClientInit(void) override;
        virtual SIPLStatus SetDataBufAttrList(void) override;
        virtual SIPLStatus SetSyncAttrList(void) override;
        virtual SIPLStatus MapDataBuffer(uint32_t packetIndex) override;
        virtual SIPLStatus RegisterSignalSyncObj(void) override;
        virtual SIPLStatus RegisterWaiterSyncObj(uint32_t index) override;
        virtual SIPLStatus InsertPrefence(uint32_t packetIndex, NvSciSyncFence &prefence) override;
        virtual SIPLStatus SetEofSyncObj(void) override;
        virtual SIPLStatus ProcessPayload(uint32_t packetIndex, NvSciSyncFence *pPostfence) override;
        virtual SIPLStatus UnregisterSyncObjs(void) override;
        virtual bool ToSkipFrame(uint32_t frameNum) override;
        virtual SIPLStatus OnProcessPayloadDone(uint32_t packetIndex) override;
        virtual bool HasCpuWait(void) {return true;};

    private:
        struct DestroyNvMediaImage
        {
            void operator ()(NvMediaImage *p) const
            {
                NvMediaImageDestroy(p);
            }
        };
        struct DestroyNvMediaIEP
        {
            void operator ()(NvMediaIEP *p) const
            {
                NvMediaIEPDestroy(p);
            }
        };

        SIPLStatus InitEncoder(void);
        SIPLStatus EncodeOneFrame(NvMediaImage *pNvMediaImage,
                                       uint8_t **ppOutputBuffer,
                                       size_t *pNumBytes,
                                       NvSciSyncFence *pPostfence);
        SIPLStatus SetEncodeConfig(void);

        std::unique_ptr<NvMediaDevice, CloseNvMediaDevice> m_pDevice {nullptr};
        std::unique_ptr<NvMediaIEP, DestroyNvMediaIEP> m_pNvMIEP {nullptr};
        NvMediaImage* m_images[MAX_PACKETS];
        FILE *m_pOutputFile = nullptr;
        NvMediaEncodeConfigH264 m_stEncodeConfigH264Params;
        NvMediaSurfaceType m_surfaceType;
        uint16_t m_encodeWidth;
        uint16_t m_encodeHeight;
        uint8_t *m_pEncodedBuf = nullptr;
        size_t m_encodedBytes;
    };
#endif
