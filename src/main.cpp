#include <fstream>
#include <iostream>
#include "cameraCalibMDC.hpp"

using namespace CameraCalib;
using namespace std;

static std::map<std::string,
                std::pair<std::string, CameraCalibMDC::mdc_camera_model_e>>  // pair{string, enum}
    cameras_map = {{"front_far", {"A1", CameraCalibMDC::IMX728}},
                   {"right_rear", {"A2", CameraCalibMDC::IMX728}},
                   {"front_fisheye", {"C1", CameraCalibMDC::tte_IMX390}},
                   {"rear_fisheye", {"C2", CameraCalibMDC::tte_IMX390}},
                   {"left_fisheye", {"C3", CameraCalibMDC::tte_IMX390}},
                   {"right_fisheye", {"C4", CameraCalibMDC::tte_IMX390}}};

long ReadData(std::string slotName, const void *readBuf_t, uint32_t maxReadLength) {
  long reads;
  if (!readBuf_t || maxReadLength <= 0 || slotName.empty()) {
    return -1;
  }


  char *readBuf = (char *)readBuf_t;  // 强转为char *

  std::string name = "./" + slotName + ".bin";  // 定位到A1-C4的几个bin
  std::ifstream stream(name, ios::in | ios::binary | ios::ate); // 二进制打开、定位到文件尾
  if (!stream.is_open()) {
    std::cout << name << " open failed!" << std::endl;
    return -1;
  }
  reads = stream.tellg(); // 读当前文件指针的位置
  if (reads > maxReadLength) reads = maxReadLength;
  stream.seekg(0, ios::beg);  // 定位到文件头
  stream.read(readBuf, reads);
  stream.close();

  return reads;
}

int main(int argc, char *argv[]) {
  for (auto it = cameras_map.begin(); it != cameras_map.end(); it++) {
    cout << it->first << ':' << it->second.first << ':' << it->second.second
         << endl;
    CameraCalibMDCPtr camera_calib =
        std::make_shared<CameraCalibMDC>(it->first);      // 创建并返回指向分配对象的 shared_ptr，这些对象是通过使用默认分配器从零个或多个参数构造的。 
    std::shared_ptr<char> EEPROMBinData(new char[20480]); // 开辟了20kb的空间以存储读取的bin
    long EEPROMBinDataSize = 0;
    EEPROMBinDataSize =
        ReadData(it->second.first, EEPROMBinData.get(), 20480 - 1); // 读取20kb以内的bin
    if (EEPROMBinDataSize <= 0) {
      cout << "read EEPROMBin failed" << endl;
      continue;
    } else
      cout << "read from bin gets " << EEPROMBinDataSize << endl;

    if (it->second.second == CameraCalibMDC::tte_IMX390) {
      camera_calib->InitCamera(1920, 1200, true);
    } else {
      camera_calib->InitCamera(3840, 2160, false);
    }
    auto calib_file_path =
        "./camera_" + it->first + ".yaml";
    camera_calib->LoadCalibFromFileYaml(calib_file_path.c_str()); // .c_str返回C语言标准的字符串数组指针
    camera_calib->LoadCalibFromEEPROM(it->second.second, EEPROMBinData.get(),
                                      EEPROMBinDataSize);
    if (camera_calib->IsCameraChanged()) cout << "camera changed ! " << endl;
    if (!camera_calib->IsCalibFileOK()) cout << "calib file broken ! " << endl;
    auto output_path = "./output_" + it->second.first + ".yaml";
    camera_calib->WriteCalibToFileYaml(output_path.c_str());
  }

  return 0;
}

