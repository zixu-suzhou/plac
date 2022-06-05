#include <fstream>
#include <iostream>
#include "cameraCalibMDC.hpp"

using namespace CameraCalib;
using namespace std;

static std::map<std::string,
                std::pair<std::string, CameraCalibMDC::mdc_camera_model_e>>
    cameras_map = {{"left_rear", {"A1", CameraCalibMDC::IMX728}},
                   {"right_rear", {"A2", CameraCalibMDC::IMX728}},
                   {"front_fisheye", {"C1", CameraCalibMDC::tte_IMX390}},
                   {"rear_fisheye", {"C2", CameraCalibMDC::tte_IMX390}},
                   {"left_fisheye", {"C3", CameraCalibMDC::tte_IMX390}},
                   {"right_fisheye", {"C4", CameraCalibMDC::tte_IMX390}}};

long ReadData(std::string slotName, char *readBuf, uint32_t maxReadLength) {
  long reads;
  if (!readBuf) {
    return -1;
  }
  std::string name = "./" + slotName + ".bin";
  std::ifstream stream(name, ios::in | ios::binary | ios::ate);
  if (!stream.is_open()) {
    std::cout << name << " open failed!" << std::endl;
    return -1;
  }
  reads = stream.tellg();
  if (reads > maxReadLength) reads = maxReadLength;
  stream.seekg(0, ios::beg);
  stream.read(readBuf, reads);
  stream.close();

  return reads;
}

int main(int argc, char *argv[]) {
  for (auto it = cameras_map.begin(); it != cameras_map.end(); it++) {
    cout << it->first << ':' << it->second.first << ':' << it->second.second
         << endl;
    CameraCalibMDCPtr camera_calib =
        std::make_shared<CameraCalibMDC>(it->first);
    std::shared_ptr<char> EEPROMBinData(new char[20480]);
    long EEPROMBinDataSize = 0;
    EEPROMBinDataSize =
        ReadData(it->second.first, EEPROMBinData.get(), 20480 - 1);
    if (EEPROMBinDataSize <= 0) {
      cout << "read EEPROMBin failed" << endl;
      continue;
    } else
      cout << "read from bin gets " << EEPROMBinDataSize << endl;

    camera_calib->InitCamera(2896, 1876, false);
    camera_calib->LoadCalibFromFileYaml(
        "/home/zhangdonghua/Downloads/camera_front_far.yaml");
    camera_calib->LoadCalibFromEEPROM(it->second.second, EEPROMBinData.get(),
                                      EEPROMBinDataSize);
    if (camera_calib->IsCameraChanged()) cout << "camera changed ! " << endl;
    if (!camera_calib->IsCalibFileOK()) cout << "calib file broken ! " << endl;
    auto output_path = "./output_" + it->second.first + ".yaml";
    camera_calib->WriteCalibToFileYaml(output_path.c_str());
  }

  return 0;
}

