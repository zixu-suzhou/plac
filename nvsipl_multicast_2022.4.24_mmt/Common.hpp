// Copyright (c) 2022 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software, related documentation and any
// modifications thereto. Any use, reproduction, disclosure or distribution
// of this software and related documentation without an express license
// agreement from NVIDIA Corporation is strictly prohibited.

#ifndef COMMON_HPP
#define COMMON_HPP
#define SUPPORT_MMT
    constexpr uint32_t NUM_LOCAL_ENC_CONSUMERS = 1U;
    constexpr uint32_t NUM_LOCAL_CUDA_CONSUMERS = 1U;
    constexpr uint32_t NUM_LOCAL_CONSUMERS = NUM_LOCAL_CUDA_CONSUMERS + NUM_LOCAL_ENC_CONSUMERS;

    constexpr uint32_t MAX_NUM_SENSORS= 16U;
    constexpr uint32_t MAX_PACKETS = 6U;
    constexpr uint32_t MAX_ELEMENTS = 2U; /* Maximum number of elements supported */
    constexpr uint32_t DATA_ELEMENT_INDEX = 0U;
    constexpr uint32_t META_ELEMENT_INDEX = 1U;
    constexpr uint32_t NUM_CONSUMERS = 6U;
    constexpr uint32_t MAX_WAIT_SYNCOBJ = NUM_CONSUMERS + NUM_LOCAL_CONSUMERS;
    constexpr uint32_t MAX_NUM_SYNCS = 8U;
    constexpr uint32_t MAX_QUERY_TIMEOUTS = 10U;
    constexpr int QUERY_TIMEOUT = 1000000; // usecs
    constexpr int QUERY_TIMEOUT_FOREVER = -1;
    constexpr uint32_t NVMEDIA_IMAGE_STATUS_TIMEOUT_MS = 100U;
    constexpr uint32_t DUMP_START_FRAME = 60U;
    constexpr uint32_t DUMP_END_FRAME = 100U;
    constexpr int64_t FENCE_FRAME_TIMEOUT_US = 100000U;
#endif
