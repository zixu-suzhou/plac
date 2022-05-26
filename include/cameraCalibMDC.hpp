#include <stdio.h>
#include <iostream>
#include "cameraCalibCommon.hpp"
namespace MDC_CameraCalib {

class CameraCalibMDC : public CameraCalibCommon {
 public:
  CameraCalibMDC(std::string camera_name) : CameraCalibCommon(camera_name){};
  uint8_t LoadCalibFromEEPROM() { return 0; };
};

using CameraCalibMDCPtr = std::shared_ptr<CameraCalibMDC>;
}  // namespace MDC_CameraCalib
