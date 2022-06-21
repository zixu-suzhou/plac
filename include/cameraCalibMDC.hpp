#include <stdio.h>
#include <iostream>
#include "cameraCalibCommon.hpp"
namespace CameraCalib {

#define CAMERA_CALIB_BASE_OFFSET 0x0835
#define CAMERA_CALIB_END_OFFSET 0x00FF

class CameraCalibMDC : public CameraCalibCommon {
 public:
  typedef enum _mdc_camera_model_e {
    tte_IMX390 = 0,
    IMX728,
    mdc_camera_model_max
  } mdc_camera_model_e;
  typedef struct TTE_EEPROMCalib_s {
    int8_t module_information[0x001f];
    int8_t reserved_1[0x0040];
    int8_t model;
    int8_t data_type;
    int8_t reserved_2[6];
    double fx;
    double fy;
    double cx;
    double cy;
    double k1;
    double k2;
    double k3;
    double k4;
  } TTE_EEPROMCalib_t;

  typedef struct IMX728_EEPROMCalib_s {
    int8_t section_type;
    int8_t intrinsics_version;
    int8_t reserved_1[3];
    int8_t data_type;
    int8_t reserved_2[0x0021];
    int8_t reprojection_error_value[8];
    int8_t reserved_3[0x000f];
    double cx;
    double cy;
    double fx;
    double fy;
    int8_t reserved_4[0x003f];
    double k1;
    double k2;
    double k3;
    double k4;
    int8_t reserved_5[0x000f];
    double p1;
    double p2;
    int8_t reserved_6[0x0017];
    int32_t image_width;
    int32_t image_height;
  } IMX728_EEPROMCalib_t;
  CameraCalibMDC(std::string camera_name) : CameraCalibCommon(camera_name){};
  uint8_t LoadCalibFromEEPROM() { return 0; }
  uint8_t LoadCalibFromEEPROM(mdc_camera_model_e camera, char *bin,
                              long length) {
    if (!bin || length <= 0){
      return -1;
    }

    if (camera == IMX728) {
      if (length < (CAMERA_CALIB_BASE_OFFSET + sizeof(IMX728_EEPROMCalib_t))){
        return -1;
      }

      char *p = bin + CAMERA_CALIB_BASE_OFFSET;
      IMX728_EEPROMCalib_t *EEPROMCalibPtr;

      EEPROMCalibPtr = (IMX728_EEPROMCalib_t *)p;
      std::cout << "EEPROMCalibBin data type " << std::hex
                << (int32_t)EEPROMCalibPtr->data_type << std::endl;
      std::cout << "load image_width from EEPROM " << std::dec
                << EEPROMCalibPtr->image_width << std::endl;
      std::cout << "load image_height from EEPROM " << std::dec
                << EEPROMCalibPtr->image_height << std::endl;
      std::cout << "load fx from EEPROM " << EEPROMCalibPtr->fx << std::endl;
      std::cout << "load fy from EEPROM " << EEPROMCalibPtr->fy << std::endl;
      std::cout << "load cx from EEPROM " << EEPROMCalibPtr->cx << std::endl;
      std::cout << "load cy from EEPROM " << EEPROMCalibPtr->cy << std::endl;
      std::cout << "load k1 from EEPROM " << EEPROMCalibPtr->k1 << std::endl;
      std::cout << "load k2 from EEPROM " << EEPROMCalibPtr->k2 << std::endl;
      std::cout << "load k3 from EEPROM " << EEPROMCalibPtr->k3 << std::endl;
      std::cout << "load k4 from EEPROM " << EEPROMCalibPtr->k4 << std::endl;
      m_EEPROMParam.fx = EEPROMCalibPtr->fx;
      m_EEPROMParam.fy = EEPROMCalibPtr->fy;
      m_EEPROMParam.cx = EEPROMCalibPtr->cx;
      m_EEPROMParam.cy = EEPROMCalibPtr->cy;
      m_EEPROMParam.k1 = EEPROMCalibPtr->k1;
      m_EEPROMParam.k2 = EEPROMCalibPtr->k2;
      m_EEPROMParam.k3 = EEPROMCalibPtr->k3;
      m_EEPROMParam.k4 = EEPROMCalibPtr->k4;

    } else if (camera == tte_IMX390) {
      if (length < (sizeof(TTE_EEPROMCalib_t))){
        return -1;
      }

      char *p = bin;
      TTE_EEPROMCalib_t *EEPROMCalibPtr;

      EEPROMCalibPtr = (TTE_EEPROMCalib_t *)p;
      std::cout << "EEPROMCalibBin data type " << std::hex
                << (int32_t)EEPROMCalibPtr->data_type << std::endl;
      if (EEPROMCalibPtr->data_type == 2) {
        std::cout << "load fx from EEPROM " << EndianSwap(EEPROMCalibPtr->fx)
                  << std::endl;
        std::cout << "load fy from EEPROM " << EndianSwap(EEPROMCalibPtr->fy)
                  << std::endl;
        std::cout << "load cx from EEPROM " << EndianSwap(EEPROMCalibPtr->cx)
                  << std::endl;
        std::cout << "load cy from EEPROM " << EndianSwap(EEPROMCalibPtr->cy)
                  << std::endl;
        std::cout << "load k1 from EEPROM " << EndianSwap(EEPROMCalibPtr->k1)
                  << std::endl;
        std::cout << "load k2 from EEPROM " << EndianSwap(EEPROMCalibPtr->k2)
                  << std::endl;
        std::cout << "load k3 from EEPROM " << EndianSwap(EEPROMCalibPtr->k3)
                  << std::endl;
        std::cout << "load k4 from EEPROM " << EndianSwap(EEPROMCalibPtr->k4)
                  << std::endl;
        m_EEPROMParam.fx =EndianSwap(EEPROMCalibPtr->fx);
        m_EEPROMParam.fy =EndianSwap(EEPROMCalibPtr->fy);
        m_EEPROMParam.cx =(double)1920 - EndianSwap(EEPROMCalibPtr->cx);
        m_EEPROMParam.cy =(double)1200 - EndianSwap(EEPROMCalibPtr->cy);
        m_EEPROMParam.k1 =EndianSwap(EEPROMCalibPtr->k1);
        m_EEPROMParam.k2 =EndianSwap(EEPROMCalibPtr->k2);
        m_EEPROMParam.k3 =EndianSwap(EEPROMCalibPtr->k3);
        m_EEPROMParam.k4 =EndianSwap(EEPROMCalibPtr->k4);

      } else {
        std::cout << "load fx from EEPROM " << EEPROMCalibPtr->fx << std::endl;
        std::cout << "load fy from EEPROM " << EEPROMCalibPtr->fy << std::endl;
        std::cout << "load cx from EEPROM " << EEPROMCalibPtr->cx << std::endl;
        std::cout << "load cy from EEPROM " << EEPROMCalibPtr->cy << std::endl;
        std::cout << "load k1 from EEPROM " << EEPROMCalibPtr->k1 << std::endl;
        std::cout << "load k2 from EEPROM " << EEPROMCalibPtr->k2 << std::endl;
        std::cout << "load k3 from EEPROM " << EEPROMCalibPtr->k3 << std::endl;
        std::cout << "load k4 from EEPROM " << EEPROMCalibPtr->k4 << std::endl;
        m_EEPROMParam.fx = EEPROMCalibPtr->fx;
        m_EEPROMParam.fy = EEPROMCalibPtr->fy;
        m_EEPROMParam.cx = EEPROMCalibPtr->cx;
        m_EEPROMParam.cy = EEPROMCalibPtr->cy;
        m_EEPROMParam.k1 = EEPROMCalibPtr->k1;
        m_EEPROMParam.k2 = EEPROMCalibPtr->k2;
        m_EEPROMParam.k3 = EEPROMCalibPtr->k3;
        m_EEPROMParam.k4 = EEPROMCalibPtr->k4;
      }
    } else {
      std::cout << "not supported camera model" << camera << std::endl;
      return -1;
    }

    DoubleToString(m_EEPROMStrParam.strfx, m_EEPROMParam.fx);
    DoubleToString(m_EEPROMStrParam.strfy, m_EEPROMParam.fy);
    DoubleToString(m_EEPROMStrParam.strcx, m_EEPROMParam.cx);
    DoubleToString(m_EEPROMStrParam.strcy, m_EEPROMParam.cy);
    DoubleToString(m_EEPROMStrParam.strk1, m_EEPROMParam.k1);
    DoubleToString(m_EEPROMStrParam.strk2, m_EEPROMParam.k2);
    DoubleToString(m_EEPROMStrParam.strk3, m_EEPROMParam.k3);
    DoubleToString(m_EEPROMStrParam.strk4, m_EEPROMParam.k4);

    return 0;
  };
  void ShowEEPROMCalib(){

  };
};

using CameraCalibMDCPtr = std::shared_ptr<CameraCalibMDC>;
}
