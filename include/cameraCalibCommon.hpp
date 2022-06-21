#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include "yaml-cpp/yaml.h"

typedef struct _Vec3 {
  double x;
  double y;
  double z;
} Vec3;
template <>
struct YAML::convert<Vec3> {
  static YAML::Node encode(const Vec3 &rhs) {
    YAML::Node node;
    node.push_back(rhs.x);
    node.push_back(rhs.y);
    node.push_back(rhs.z);
    return node;
  }

  static bool decode(const YAML::Node &node, Vec3 &rhs) {
    if (!node.IsSequence() || node.size() != 3) {
      return false;
    }

    rhs.x = node[0].as<double>();
    rhs.y = node[1].as<double>();
    rhs.z = node[2].as<double>();
    return true;
  }
};

typedef struct _Roi {
  int x0;
  int y0;
  int x1;
  int y1;
} Roi;
template <>
struct YAML::convert<Roi> {
  static YAML::Node encode(const Roi &rhs) {
    YAML::Node node;
    node.push_back(rhs.x0);
    node.push_back(rhs.y0);
    node.push_back(rhs.x1);
    node.push_back(rhs.y1);
    return node;
  }

  static bool decode(const YAML::Node &node, Roi &rhs) {
    if (!node.IsSequence() || node.size() != 4) {
      return false;
    }
    rhs.x0 = node[0].as<int>();
    rhs.y0 = node[1].as<int>();
    rhs.x1 = node[2].as<int>();
    rhs.y1 = node[3].as<int>();
    return true;
  }
};

namespace CameraCalib {

typedef struct _CALIB_PARA {
  double fx;
  double fy;
  double cx;
  double cy;
  double dummy1;
  double dummy2;
  double dummy3;
  double k1;
  double k2;
  double dummy4;
  double dummy5;
  double k3;
  double k4;
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
  uint8_t InitCamera(uint32_t width, uint32_t height, bool is_fisheye) {
    m_imageWidth = width;
    m_imageHeight = height;
    m_isFisheye = is_fisheye;

    return 0;
  };
  void InitYAMLNode() {
    Vec3 s2b = {0.0, 0.0, 0.0};
    Roi roi = {0, 0, 0, 0};
    m_yamlNode["CLOCK_calib_version"] = "";
    m_yamlNode["CLOCK_calib_details"] = "";
    m_yamlNode["CLOCK_calib_date"] = "";
    m_yamlNode["sensor_name"] =
        std::string("camera_") + std::string(m_cameraName);
    m_yamlNode["sensor_type"] = "camera";
    m_yamlNode["timestamp_shift"] = 0;
    m_yamlNode["vehicle_xyz"] = "";
    m_yamlNode["r_s2b"] = s2b;
    m_yamlNode["t_s2b"] = s2b;
    m_yamlNode["camera_model"] = "polyn";
    m_yamlNode["fx"] = 0.0;
    m_yamlNode["fy"] = 0.0;
    m_yamlNode["cx"] = 0.0;
    m_yamlNode["cy"] = 0.0;
    m_yamlNode["kc2"] = 0.0;
    m_yamlNode["kc3"] = 0.0;
    m_yamlNode["kc4"] = 0.0;
    m_yamlNode["kc5"] = 0.0;
    m_yamlNode["is_fisheye"] = m_isFisheye;
    m_yamlNode["line_exposure_delay"] = 0;
    m_yamlNode["width"] = m_imageWidth;
    m_yamlNode["height"] = m_imageHeight;
    m_yamlNode["suggested_rect_region_within_ROI"] = roi;
    m_yamlNode["suggested_diagonal_FOV_within_ROI"] = "N/A";
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
    if (!AccessCalibFile(file_to_load)) {
      InitYAMLNode();
      std::cout << "not exist calib yaml file" << std::endl;
      return -1;
    }
    m_yamlNode = YAML::LoadFile(file_to_load.c_str());
    std::cout << "finish yaml file load" << std::endl;
    if (m_yamlNode["fx"] && !m_yamlNode["fx"].as<std::string>().empty() &&
        (m_yamlNode["fx"].as<std::string>() != std::string("-nan"))) {
      m_fileStrParam.strfx = m_yamlNode["fx"].as<std::string>();
      m_fileParam.fx = m_yamlNode["fx"].as<double>();
    }
    std::cout << "load fx from file " << m_fileStrParam.strfx << std::endl;
    if (m_yamlNode["fy"] && !m_yamlNode["fy"].as<std::string>().empty() &&
        (m_yamlNode["fy"].as<std::string>() != std::string("-nan"))) {
      m_fileStrParam.strfy = m_yamlNode["fy"].as<std::string>();
      m_fileParam.fy = m_yamlNode["fy"].as<double>();
    }
    std::cout << "load fy from file " << m_fileStrParam.strfy << std::endl;
    if (m_yamlNode["cx"] && !m_yamlNode["cx"].as<std::string>().empty() &&
        (m_yamlNode["cx"].as<std::string>() != std::string("-nan"))) {
      m_fileStrParam.strcx = m_yamlNode["cx"].as<std::string>();
      m_fileParam.cx = m_yamlNode["cx"].as<double>();
    }
    std::cout << "load cx from file " << m_fileStrParam.strcx << std::endl;
    if (m_yamlNode["cy"] && !m_yamlNode["cy"].as<std::string>().empty() &&
        (m_yamlNode["cy"].as<std::string>() != std::string("-nan"))) {
      m_fileStrParam.strcy = m_yamlNode["cy"].as<std::string>();
      m_fileParam.cy = m_yamlNode["cy"].as<double>();
    }
    std::cout << "load cy from file " << m_fileStrParam.strcy << std::endl;
    if (m_yamlNode["kc2"] && !m_yamlNode["kc2"].as<std::string>().empty() &&
        (m_yamlNode["kc2"].as<std::string>() != std::string("-nan"))) {
      m_fileStrParam.strk1 = m_yamlNode["kc2"].as<std::string>();
      m_fileParam.k1 = m_yamlNode["kc2"].as<double>();
    }
    std::cout << "load k1 from file " << m_fileStrParam.strk1 << std::endl;
    if (m_yamlNode["kc3"] && !m_yamlNode["kc3"].as<std::string>().empty() &&
        (m_yamlNode["kc3"].as<std::string>() != std::string("-nan"))) {
      m_fileStrParam.strk2 = m_yamlNode["kc3"].as<std::string>();
      m_fileParam.k2 = m_yamlNode["kc3"].as<double>();
    }
    std::cout << "load k2 from file " << m_fileStrParam.strk2 << std::endl;
    if (m_yamlNode["kc4"] && !m_yamlNode["kc4"].as<std::string>().empty() &&
        (m_yamlNode["kc4"].as<std::string>() != std::string("-nan"))) {
      m_fileStrParam.strk3 = m_yamlNode["kc4"].as<std::string>();
      m_fileParam.k3 = m_yamlNode["kc4"].as<double>();
    }
    std::cout << "load k3 from file " << m_fileStrParam.strk3 << std::endl;
    if (m_yamlNode["kc5"] && !m_yamlNode["kc5"].as<std::string>().empty() &&
        (m_yamlNode["kc5"].as<std::string>() != std::string("-nan"))) {
      m_fileStrParam.strk4 = m_yamlNode["kc5"].as<std::string>();
      m_fileParam.k4 = m_yamlNode["kc5"].as<double>();
    }
    std::cout << "load k4 from file " << m_fileStrParam.strk4 << std::endl;

    std::cout << "LoadCalibFromFileYaml return 0" << std::endl;
    return 0;
  };

  uint8_t WriteCalibToFileYaml(std::string file_to_save) {
    std::ofstream camera_calib_output_file(file_to_save);
    if (!camera_calib_output_file.is_open()) {
      return -1;
    }
    YAML::Emitter yaml_emitter(camera_calib_output_file);
    camera_calib_output_file << std::string("%YAML:1.0\n");
    camera_calib_output_file << "---\n";
    yaml_emitter << YAML::BeginMap;
    yaml_emitter << YAML::Key << "CLOCK_calib_version" << YAML::Value
                 << YAML::DoubleQuoted << m_yamlNode["CLOCK_calib_version"];
    yaml_emitter << YAML::Key << "CLOCK_calib_details" << YAML::Value
                 << YAML::DoubleQuoted << m_yamlNode["CLOCK_calib_details"];
    yaml_emitter << YAML::Key << "CLOCK_calib_date" << YAML::Value
                 << YAML::DoubleQuoted << m_yamlNode["CLOCK_calib_date"];

    yaml_emitter << YAML::Key << "sensor_name" << YAML::Value
                 << YAML::DoubleQuoted << m_yamlNode["sensor_name"]
                 << YAML::Comment("传感器命名");
    yaml_emitter << YAML::Key << "sensor_type" << YAML::Value
                 << YAML::DoubleQuoted << m_yamlNode["sensor_type"]
                 << YAML::Comment("传感器类型");
    yaml_emitter << YAML::Key << "timestamp_shift" << YAML::Value
                 << m_yamlNode["timestamp_shift"]
                 << YAML::Comment(
                        "时间戳延时, 单位 ms, 真实时间戳 = 得到时间戳 + "
                        "timestamp_shift");

    yaml_emitter
        << YAML::Key << "vehicle_xyz" << YAML::Value << YAML::DoubleQuoted
        << m_yamlNode["vehicle_xyz"]
        << YAML::Comment(
               "车体系定义, 前左上, 后轴中心接地点"); 

    yaml_emitter
        << YAML::Key << "r_s2b"
        << YAML::Comment(
               "传感器到车体系的旋转, i.e. p_b = R(r_s2b)*p_s, "
               "p_b表示车体系下的点, "
               "R(.)表示把旋转向量转换成旋转矩阵的函数, p_s表示传感器系的点")
        << YAML::Flow << m_yamlNode["r_s2b"];

    yaml_emitter << YAML::Key << "t_s2b"
                 << YAML::Comment("传感器到车体的平移, 单位 m") << YAML::Flow
                 << m_yamlNode["t_s2b"];

    yaml_emitter << YAML::Key << "camera_model" << YAML::Value
                 << YAML::DoubleQuoted << m_yamlNode["camera_model"]
                 << YAML::Comment("相机模型-poly, 也叫等距相机模型");

    yaml_emitter << YAML::Key << "fx" << YAML::Value << m_EEPROMStrParam.strfx
                 << YAML::Comment("内参-焦距-fx, 单位 像素");
    yaml_emitter << YAML::Key << "fy" << YAML::Value << m_EEPROMStrParam.strfy
                 << YAML::Comment("内参-焦距-fy, 单位 像素");
    yaml_emitter << YAML::Key << "cx" << YAML::Value << m_EEPROMStrParam.strcx
                 << YAML::Comment("内参-主点-cx, 单位 像素");
    yaml_emitter << YAML::Key << "cy" << YAML::Value << m_EEPROMStrParam.strcy
                 << YAML::Comment("内参-主点-cy, 单位 像素");
    yaml_emitter << YAML::Key << "kc2" << YAML::Value << m_EEPROMStrParam.strk1
                 << YAML::Comment("内参-畸变系数-kc2");
    yaml_emitter << YAML::Key << "kc3" << YAML::Value << m_EEPROMStrParam.strk2
                 << YAML::Comment("内参-畸变系数-kc3");
    yaml_emitter << YAML::Key << "kc4" << YAML::Value << m_EEPROMStrParam.strk3
                 << YAML::Comment("内参-畸变系数-kc4");
    yaml_emitter << YAML::Key << "kc5" << YAML::Value << m_EEPROMStrParam.strk4
                 << YAML::Comment("内参-畸变系数-kc5");

    yaml_emitter << YAML::Key << "is_fisheye" << YAML::Value
                 << m_yamlNode["is_fisheye"] << YAML::Comment("是否是鱼眼相机");
    yaml_emitter << YAML::Key << "line_exposure_delay" << YAML::Value
                 << m_yamlNode["line_exposure_delay"]
                 << YAML::Comment("行曝光延迟, 单位 us");
    yaml_emitter << YAML::Key << "width" << YAML::Value << m_imageWidth
                 << YAML::Comment("图像宽度, 单位 像素");
    yaml_emitter << YAML::Key << "height" << YAML::Value << m_imageHeight
                 << YAML::Comment("图像高度, 单位 像素");
    yaml_emitter << YAML::Key << "suggested_rect_region_within_ROI"
                 << YAML::Flow << m_yamlNode["suggested_rect_region_within_ROI"]
                 << YAML::Comment("建议使用的图像ROI, 单位 像素");
    yaml_emitter << YAML::Key << "suggested_diagonal_FOV_within_ROI"
                 << YAML::Value << YAML::DoubleQuoted
                 << m_yamlNode["suggested_diagonal_FOV_within_ROI"]
                 << YAML::Comment("建议使用的ROI, 单位 deg");

    yaml_emitter << YAML::EndMap;

    camera_calib_output_file.close();

    return 0;
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
  bool IsdoubleE(double a, double b) {
    if (fabs(a - b) < 1e-4)
      return true;
    else
      return false;
  };

  bool IsCalibFileOK() {
    if (IsdoubleE(m_fileParam.fx, m_EEPROMParam.fx) &&
        IsdoubleE(m_fileParam.fy, m_EEPROMParam.fy) &&
        IsdoubleE(m_fileParam.k1, m_EEPROMParam.k1) &&
        IsdoubleE(m_fileParam.k2, m_EEPROMParam.k2) &&
        IsdoubleE(m_fileParam.k3, m_EEPROMParam.k3) &&
        IsdoubleE(m_fileParam.k4, m_EEPROMParam.k4)) {
      return true;
    } else
      return false;
  };

  void FloatToString(std::string &strdst, float src) {
    char tmp[64];
    sprintf(tmp, "%f", src);
    strdst = tmp;
  };

  void DoubleToString(std::string &strdst, double src) {
    char tmp[128];
    sprintf(tmp, "%.15lf", src);
    strdst = tmp;
  };

  double EndianSwap(double d) {
    char ch[8];
    memcpy(ch, &d, 8);
    ch[0] ^= ch[7] ^= ch[0] ^= ch[7];
    ch[1] ^= ch[6] ^= ch[1] ^= ch[6];
    ch[2] ^= ch[5] ^= ch[2] ^= ch[5];
    ch[3] ^= ch[4] ^= ch[3] ^= ch[4];

    double dRet;
    memcpy(&dRet, ch, 8);
    return dRet;
  };

 protected:
  uint32_t m_imageWidth;
  uint32_t m_imageHeight;
  bool m_isFisheye;

  YAML::Node m_yamlNode;
  CALIB_PARA m_fileParam;
  STRING_CALIB_PARA m_fileStrParam;
  CALIB_PARA m_EEPROMParam;
  STRING_CALIB_PARA m_EEPROMStrParam;

 private:
  std::string m_cameraName;
};
}  
