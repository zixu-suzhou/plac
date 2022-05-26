#include <stdio.h>
#include <iostream>
#include "cameraCalibCommon.hpp"
namespace CameraCalib {

class CameraCalibMDC : public CameraCalibCommon {
 public:
  CameraCalibMDC(std::string camera_name) : CameraCalibCommon(camera_name){};
  uint8_t LoadCalibFromEEPROM() {
    m_EEPROMParam.fx = 6666.244629;
    m_EEPROMParam.fy = 7777.801758;
    m_EEPROMParam.cx = 8888.070557;
    m_EEPROMParam.cy = 9999.875000;
    m_EEPROMParam.k1 = 0.111111;
    m_EEPROMParam.k2 = -0.222222;
    m_EEPROMParam.k3 = 0.000000;
    m_EEPROMParam.k4 = 0.000000;
    FloatToString(m_EEPROMStrParam.strfx, m_EEPROMParam.fx);
    FloatToString(m_EEPROMStrParam.strfy, m_EEPROMParam.fy);
    FloatToString(m_EEPROMStrParam.strcx, m_EEPROMParam.cx);
    FloatToString(m_EEPROMStrParam.strcy, m_EEPROMParam.cy);
    FloatToString(m_EEPROMStrParam.strk1, m_EEPROMParam.k1);
    FloatToString(m_EEPROMStrParam.strk2, m_EEPROMParam.k2);
    FloatToString(m_EEPROMStrParam.strk3, m_EEPROMParam.k3);
    FloatToString(m_EEPROMStrParam.strk4, m_EEPROMParam.k4);

    return 0;
  };
  void ShowEEPROMCalib(){

  };
};

using CameraCalibMDCPtr = std::shared_ptr<CameraCalibMDC>;
}  // namespace CameraCalib
