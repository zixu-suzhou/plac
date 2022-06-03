#include <fstream>
#include <iostream>
#include "cameraCalibMDC.hpp"

using namespace CameraCalib;
using namespace std;

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
  CameraCalibMDCPtr camera_calib =
      std::make_shared<CameraCalibMDC>("front_far");
  std::shared_ptr<char> A1_EEPROMBinData(new char[20480]);
  long A1_EEPROMBinDataSize = 0;

  cout << "camera tag is " << camera_calib->GetCameraTag() << endl;
  camera_calib->InitCamera(2896, 1876, false);
  camera_calib->LoadCalibFromFileYaml(
      "/home/zhangdonghua/Downloads/camera_front_far.yaml");
  camera_calib->LoadCalibFromEEPROM();
  if (camera_calib->IsCameraChanged()) cout << "camera changed ! " << endl;
  if (!camera_calib->IsCalibFileOK()) cout << "calib file broken ! " << endl;
  camera_calib->WriteCalibToFileYaml("./output.yaml");

  A1_EEPROMBinDataSize = ReadData("A1", A1_EEPROMBinData.get(), 20480 - 1);
  if (A1_EEPROMBinDataSize > 0) {
    cout << "read from bin gets " << A1_EEPROMBinDataSize << endl;
    for (int i = 0; i < A1_EEPROMBinDataSize; i++) {
      cout << std::hex << (int)*(A1_EEPROMBinData.get() + i) << ' ';
    }
    cout << endl;
  } else
    cout << "read EEPROMBin failed" << endl;

  return 0;
}

