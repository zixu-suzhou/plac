#include <fstream>
#include <stdio.h>
#include <iostream>
#include "yaml-cpp/yaml.h"

namespace MDC_CameraCalib {

typedef struct _CALIB_PARA {
  float fx;
  float fy;
  float cx;
  float cy;
  float dummy1;
  float dummy2;
  float dummy3;
  float k1;
  float k2;
  float dummy4;
  float dummy5;
  float k3;
  float k4;
} CALIB_PARA;
typedef struct _STRING_CALIB_PARA {
  std::string strfx;
  std::string strfy;
  std::string strcx;
  std::string strcy;
  std::string strk1;
  std::string strk2;
  std::string strk3;
  std::string strk4;
} STRING_CALIB_PARA;

class CameraCalibCommon {
 public:
  CameraCalibCommon(std::string camera_name) { m_cameraName = camera_name; };
  ~CameraCalibCommon(){};

  std::string GetCameraTag() { return m_cameraName; };
  uint8_t InitCamera(uint32_t width, uint32_t height) {
    m_imageWidth = width;
    m_imageHeight = height;

    return 0;
  };
  bool AccessCalibFile(std::string path) {
    if (FILE *file = fopen(path.c_str(), "r")) {
      fclose(file);
      return true;
    } else {
      return false;
    }
  }

  uint8_t LoadCalibFromFileYaml(std::string file_to_load) {
    if (!AccessCalibFile("hahaha")) {
      return -1;
    }
    YAML::Node config = YAML::LoadFile(file_to_load);
    m_fileStrParam.strfx = config["fx"].as<std::string>();
    m_fileStrParam.strfy = config["fy"].as<std::string>();
    m_fileStrParam.strcx = config["cx"].as<std::string>();
    m_fileStrParam.strcy = config["cy"].as<std::string>();
    m_fileStrParam.strk1 = config["k1"].as<std::string>();
    m_fileStrParam.strk2 = config["k2"].as<std::string>();
    m_fileStrParam.strk3 = config["k3"].as<std::string>();
    m_fileStrParam.strk4 = config["k4"].as<std::string>();

    return 0;
  };
  uint8_t WriteCalibToFileYaml(std::string file_to_save) {
    std::ofstream fout(file_to_save.c_str());
    if (!fout.is_open()) {
      return -1;
    }

    YAML::Emitter ofile(fout);
    fout << std::string("%YAML:1.0\n");
    fout << "---\n";
    ofile << YAML::BeginMap;
    ofile << YAML::Key << "cameraName" << YAML::Value << YAML::DoubleQuoted
          << m_cameraName;
    ofile << YAML::Key << "cameraModel" << YAML::Value << YAML::DoubleQuoted
          << "polyn";

    ofile << YAML::Key << "width" << YAML::Value << YAML::DoubleQuoted
          << m_imageWidth << YAML::Comment("图像宽度, 单位 像素");
    ofile << YAML::Key << "height" << YAML::Value << YAML::DoubleQuoted
          << m_imageHeight << YAML::Comment("图像高度, 单位 像素");

    ofile << YAML::Key << "fx" << YAML::Value << m_EEPROMStrParam.strfx
          << YAML::Comment("内参-焦距-fx, 单位 像素");
    ofile << YAML::Key << "fy" << YAML::Value << m_EEPROMStrParam.strfy
          << YAML::Comment("内参-焦距-fy, 单位 像素");
    ofile << YAML::Key << "cx" << YAML::Value << m_EEPROMStrParam.strcx
          << YAML::Comment("内参-主点-cx, 单位 像素");
    ofile << YAML::Key << "cy" << YAML::Value << m_EEPROMStrParam.strcy
          << YAML::Comment("内参-主点-cy, 单位 像素");
    ofile << YAML::Key << "k1" << YAML::Value << m_EEPROMStrParam.strk1
          << YAML::Comment("内参-畸变系数-k1");
    ofile << YAML::Key << "k2" << YAML::Value << m_EEPROMStrParam.strk2
          << YAML::Comment("内参-畸变系数-k2");
    ofile << YAML::Key << "k3" << YAML::Value << m_EEPROMStrParam.strk3
          << YAML::Comment("内参-畸变系数-k3");
    ofile << YAML::Key << "k4" << YAML::Value << m_EEPROMStrParam.strk4
          << YAML::Comment("内参-畸变系数-k4");

    ofile << YAML::EndMap;
    fout.close();
  };
  uint8_t UpdateChangeFlagToCalibFile();
  virtual uint8_t LoadCalibFromEEPROM() = 0;
  virtual uint8_t LoadCalibFromFileCustom(){};
  virtual uint8_t WriteCalibToFileCustom(){};
  bool IsCameraChanged() {
    if (m_fileStrParam.strfx == m_EEPROMStrParam.strfx &&
        m_fileStrParam.strfy == m_EEPROMStrParam.strfy &&
        m_fileStrParam.strk1 == m_EEPROMStrParam.strk1 &&
        m_fileStrParam.strk2 == m_EEPROMStrParam.strk2 &&
        m_fileStrParam.strk3 == m_EEPROMStrParam.strk3 &&
        m_fileStrParam.strk4 == m_EEPROMStrParam.strk4 &&
        m_fileStrParam.strcx == m_EEPROMStrParam.strcx &&
        m_fileStrParam.strcy == m_EEPROMStrParam.strcy) {
      return false;
    } else {
      return true;
    }
  };

 protected:
  uint32_t m_imageWidth;
  uint32_t m_imageHeight;

  CALIB_PARA m_fileParam;
  STRING_CALIB_PARA m_fileStrParam;
  CALIB_PARA m_EEPROMParam;
  STRING_CALIB_PARA m_EEPROMStrParam;

 private:
  void floatToString(std::string &strdst, float src) {
    char tmp[64];
    sprintf(tmp, "%f", src);
    strdst = tmp;
  }

  std::string m_cameraName;
};
}  // namespace MDC_CameraCalib
