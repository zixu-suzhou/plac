nvsipl_multicast Sample App - README
Copyright (c) 2022 NVIDIA Corporation. All rights reserved.
NVIDIA Corporation and its licensors retain all intellectual property and proprietary rights in and to this software, related documentation and any modifications thereto. Any use, reproduction, disclosure or distribution of this software and related documentation without an express license agreement from NVIDIA Corporation is strictly prohibited.

---
In nvsipl_multicast sample, there is one NvMedia producer and two consumers (CUDA consumer + encoder consumer).
<V1.0>
1. Support both single process and IPC scenarios.
2. Support multiple cameras
3. Support dumping bitstreams to h264 file on encoder consumer side.
4. Support dumping frames to YUV files on CUDA consumer side.

<V1.1>
1. Support skip specific frames on each consumer side.
2. Add NvMediaImageGetStatus to wait ISP processing done in main to fix the green stripe issue.

<V1.2>
1. Replace NvMediaImageGetStatus with CPU wait to fix the green stripe issue.
2. Add cpu wait before dumping images or bitstream.
3. Support carry meta data to each consumer.
   - Currently, only frameCaptureTSC is included in the meta data.
4. Perform CPU wait after producer receives PacketReady event to WAR the issue of failing to register sync object with ISP.

Please note, you need to prepare a platform configuration header file and put it under the platform directory.
Examples of how to run the sample application:
Usage:
./nvsipl_multicast -h (detailed usage information)
./nvsipl_multicast (single process)
./nvsipl_multicast -p (IPC, start producer process.)
./nvsipl_multicast -c “cuda” (IPC, start CUDA process.)
./nvsipl_multicast -c “enc”  (IPC, start encoder process.)





