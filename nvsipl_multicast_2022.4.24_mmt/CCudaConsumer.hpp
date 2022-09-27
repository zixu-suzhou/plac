// Copyright (c) 2022 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software, related documentation and any
// modifications thereto. Any use, reproduction, disclosure or distribution
// of this software and related documentation without an express license
// agreement from NVIDIA Corporation is strictly prohibited.

#ifndef CCUDACONSUMER_H
#define CCUDACONSUMER_H

#include "CConsumer.hpp"

// cuda includes
#include "cuda_runtime_api.h"
#include "cuda.h"

#define NUM_PLANES (3U)

class CCudaConsumer: public CConsumer
{
    public:
        CCudaConsumer() = delete;
        CCudaConsumer(NvSciStreamBlock handle, uint32_t uSensor, NvSciStreamBlock queueHandle);
        virtual ~CCudaConsumer(void);

    protected:
        virtual SIPLStatus HandleClientInit(void) override;
        virtual SIPLStatus SetDataBufAttrList(void) override;
        virtual SIPLStatus SetSyncAttrList(void) override;
        virtual SIPLStatus MapDataBuffer(uint32_t packetIndex) override;
        virtual SIPLStatus RegisterSignalSyncObj(void) override;
        virtual SIPLStatus RegisterWaiterSyncObj(uint32_t index) override;
        virtual SIPLStatus InsertPrefence(uint32_t packetIndex, NvSciSyncFence &prefence) override;
        virtual SIPLStatus ProcessPayload(uint32_t packetIndex, NvSciSyncFence *pPostfence) override;
        virtual SIPLStatus OnProcessPayloadDone(uint32_t packetIndex) override;
        virtual SIPLStatus UnregisterSyncObjs(void) override;
        virtual bool HasCpuWait(void) {return true;};
    private:
        SIPLStatus InitCuda(void);
        SIPLStatus BlToPlConvert(uint32_t packetIndex, void *dstptr);

        typedef struct {
            NvSciBufType bufType;
            uint64_t size;
            uint32_t planeCount;
            NvSciBufAttrValImageLayoutType layout;
            uint32_t planeWidths[NUM_PLANES];
            uint32_t planeHeights[NUM_PLANES];
            uint32_t planePitches[NUM_PLANES];
            uint32_t planeBitsPerPixels[NUM_PLANES];
            uint32_t planeAlignedHeights[NUM_PLANES];
            uint64_t planeAlignedSizes[NUM_PLANES];
            uint64_t planeOffsets[NUM_PLANES];
            NvSciBufAttrValColorFmt planeColorFormats[NUM_PLANES];
            uint8_t planeChannelCounts[NUM_PLANES];
        } BufferAttrs;

        int m_cudaDeviceId = 0;
        uint8_t *m_pCudaCopyMem[MAX_PACKETS];
        void  *m_devPtr[MAX_PACKETS];
        cudaExternalMemory_t m_extMem[MAX_PACKETS];
        cudaStream_t m_streamWaiter = nullptr;
        cudaExternalSemaphore_t m_signalerSem;
        cudaExternalSemaphore_t m_waiterSem;
        BufferAttrs m_bufAttrs[MAX_PACKETS];
        cudaMipmappedArray_t m_mipmapArray[MAX_PACKETS][NUM_PLANES];
        cudaArray_t  m_mipLevelArray[MAX_PACKETS][NUM_PLANES];

        FILE *m_pOutputFile = nullptr;
        uint8_t *m_pHostBuf = nullptr;
        size_t m_hostBufLen;
    };
#endif
