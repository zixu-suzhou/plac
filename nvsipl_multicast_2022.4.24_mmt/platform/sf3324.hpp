/*
 * Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef SF3324_HPP
#define SF3324_HPP

static PlatformCfg platformCfgSf3324 = {
    .platform = "SF3324_CPHY_x4",
    .platformConfig = "SF3324_CPHY_x4",
    .description = "SF3324 module in 4 lane CPHY mode",
    .numDeviceBlocks = 1U,
    .deviceBlockList = {
        {
            .csiPort = NVMEDIA_IMAGE_CAPTURE_CSI_INTERFACE_TYPE_CSI_AB,
            .phyMode = NVMEDIA_ICP_CSI_CPHY_MODE,
            .i2cDevice = 0U,
            .deserInfo = {
                .name = "MAX96712",
#if !NV_IS_SAFETY
                .description = "Maxim 96712 Aggregator",
#endif // !NV_IS_SAFETY
                .i2cAddress = 0x29,
                .errGpios = {0},
                .useCDIv2API = true
            },
            .numCameraModules = 1U,
            .cameraModuleInfoList = {
                {
                    .name = "SF3324",
#if !NV_IS_SAFETY
                    .description = "Sekonix SF3324 module - 120-deg FOV, DVP AR0231-RCCB, MAX96705",
#endif // !NV_IS_SAFETY
                    .linkIndex = 0U,
                    .serInfo = {
                        .name = "MAX96705",
#if !NV_IS_SAFETY
                        .description = "Maxim 96705 Serializer",
#endif // !NV_IS_SAFETY
                        .i2cAddress = 0x40
                    },
                    .isEEPROMSupported = false,
                    .eepromInfo = {
                    },
                    .sensorInfo = {
                            .id = 0U,
                            .name = "AR0231",
#if !NV_IS_SAFETY
                            .description = "OnSemi AR0231 Sensor",
#endif // !NV_IS_SAFETY
                            .i2cAddress = 0x10,
                            .vcInfo = {
                                    .cfa = NVM_SURF_ATTR_COMPONENT_ORDER_CRBC,
                                    .embeddedTopLines = 24U,
                                    .embeddedBottomLines = 4U,
                                    .inputFormat = NVMEDIA_IMAGE_CAPTURE_INPUT_FORMAT_TYPE_RAW12,
                                    .resolution = {
                                        .width = 1920U,
                                        .height = 1208U
                                    },
                                    .fps = 30.0,
                                    .isEmbeddedDataTypeEnabled = false
                            },
                            .isTriggerModeEnabled = true
                    }
                }
            },
            .desI2CPort = 0U,
            .desTxPort = UINT32_MAX,
            .pwrPort = 0U,
            .dphyRate = {2500000U, 2500000U},
            .cphyRate = {2000000U, 1700000U}
        }
    }
};

#endif // SF3324_HPP
