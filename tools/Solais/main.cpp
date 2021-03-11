//
// Created by liuzikai on 3/11/21.
//

#include "Camera.h"
#include "ArmorDetector.h"
#include "DetectorTuner.h"

using namespace meta;

Camera camera;
ArmorDetector armorDetector;
DetectorTuner detectorTuner(&armorDetector, &camera);

int main(int argc, char *argv[]) {

    return 0;
}