#include <fstream>
#include <iostream>
#include "cameraCalibMDC.hpp"

using namespace CameraCalib;
using namespace std;

int main(int argc, char *argv[]) {
  CameraCalibMDCPtr camera_calib =
      std::make_shared<CameraCalibMDC>("front_far");

  cout << "camera tag is " << camera_calib->GetCameraTag() << endl;
  camera_calib->InitCamera(2896, 1876, false);
  camera_calib->LoadCalibFromFileYaml(
      "/home/zhangdonghua/Downloads/camera_front_far.yaml");
  camera_calib->LoadCalibFromEEPROM();
  if (camera_calib->IsCameraChanged()) cout << "camera changed ! " << endl;
  if (!camera_calib->IsCalibFileOK()) cout << "calib file broken ! " << endl;
  camera_calib->WriteCalibToFileYaml("./output.yaml");
  return 0;
}

