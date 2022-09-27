// Copyright (c) 2022 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software, related documentation and any
// modifications thereto. Any use, reproduction, disclosure or distribution
// of this software and related documentation without an express license
// agreement from NVIDIA Corporation is strictly prohibited.

#ifndef CPOOLMANAGER_HPP
#define CPOOLMANAGER_HPP

#include "nvscistream.h"
#include "Common.hpp"
#include "CUtils.hpp"
#include "CEventHandler.hpp"

// Define attributes of a packet element.
typedef struct {
    uint32_t userName;  /* The application's name for the element */
    NvSciBufAttrList bufAttrList = nullptr;
} ElemAttr;

class CPoolManager: public CEventHandler
{
public:
    CPoolManager(NvSciStreamBlock handle, uint32_t uSensor);
    ~CPoolManager(void);

    SIPLStatus Init(void);
    virtual EventStatus HandleEvents(void) override;
    static SIPLStatus ReconcileAndAllocBuffers(NvSciBufAttrList& bufAttrList, NvSciBufObj *pInputBuffers);

private:
    SIPLStatus HandlePoolBufferSetup(void);
    SIPLStatus HandlePacketsStatus(void);

    uint32_t            m_numConsumers;
    // Producer packet element attributue
    uint32_t            m_numProdElem = 0U;
    ElemAttr            m_prodElems[MAX_ELEMENTS];

    // Consumer packet element attributue
    uint32_t            m_numConsElem = 0U;
    ElemAttr            m_consElems[MAX_ELEMENTS];

    // Reconciled packet element atrribute
    uint32_t            m_numElem = 0U;
    ElemAttr            m_elems[MAX_ELEMENTS];

    // Packet element descriptions
    NvSciStreamPacket   m_packetHandles[MAX_PACKETS];

    uint32_t            m_numPacketReady;
    bool                m_elementsDone;
    bool                m_packetsDone;
};

#endif
