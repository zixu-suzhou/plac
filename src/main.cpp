#include <fstream>
#include <iostream>
#include "cameraCalibMDC.hpp"

using namespace MDC_CameraCalib;
using namespace std;

int main(int argc, char *argv[]) {
  CameraCalibMDCPtr camera_calib =
      std::make_shared<CameraCalibMDC>("front_far");

  cout << "camera tag is " << camera_calib->GetCameraTag() << endl;
  camera_calib->InitCamera(2896, 1876);

  return 0;
}

