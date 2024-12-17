#include "CameraController.h"

#include "Core/Math.h"
#include "Math.h"
#include "Utils/Logger.h"
#include "Window.h"
namespace Rastery {

constexpr float3 kToCameraBase = float3(0, 0, 1);

float2 convertCamPosRange(const float2 pos) {
    // Convert [0,1] range to [-1, 1], and inverse the Y (screen-space y==0 is top)
    const float2 scale(2, -2);
    const float2 offset(-1, 1);
    float2 res = (pos * scale) + offset;
    return res;
}

float3 project2DCrdToUnitSphere(float2 xy) {
    float xyLengthSquared = dot(xy, xy);

    float z = 0;
    if (xyLengthSquared < 1) {
        z = std::sqrt(1 - xyLengthSquared);
    } else {
        xy = normalize(xy);
    }
    return float3(xy.x, xy.y, z);
}

OrbiterCameraController::OrbiterCameraController(const Camera::SharedPtr& pCam) : mpCamera(pCam) {
    // Compute orbit control data from camera
    const CameraData& data = pCam->getData();
    float3 toCamera = data.posW - data.target;

    mDistToFocus = length(toCamera);
    toCamera /= mDistToFocus;
    mRotation = matrixFromQuat(quatFromRotationBetweenVectors(kToCameraBase, toCamera));
}
void OrbiterCameraController::onMouseEvent(const MouseEvent& mouseEvent) {
    switch (mouseEvent.type) {
        case MouseEvent::Type::ButtonDown: {
            mIsLeftButtonDown = true;
            mLastVec = project2DCrdToUnitSphere(convertCamPosRange(mouseEvent.pos));
        } break;
        case MouseEvent::Type::ButtonUp: {
            mIsLeftButtonDown = false;
        } break;
        case MouseEvent::Type::Move:
            if (mIsLeftButtonDown) {
                float3 curVec = project2DCrdToUnitSphere(convertCamPosRange(mouseEvent.pos));
                auto q = quatFromRotationBetweenVectors(mLastVec, curVec);
                auto rot = matrixFromQuat(q);
                mRotation = rot * mRotation;
                mLastVec = curVec;
            }
            break;

        case MouseEvent::Type::Wheel: {
            mDistToFocus = std::max(1e-3f, mDistToFocus - mouseEvent.wheelDelta.y * 0.1f * mModelRadius);
        } break;
    }
}

void OrbiterCameraController::update() {
    float3 toCamera = mRotation * kToCameraBase;
    toCamera *= mDistToFocus;
    mpCamera->setPositionWorld(mpCamera->getData().target + toCamera);
    float3 up(0, 1, 0);
    up = mRotation * up;
    mpCamera->setUpVec(up);
}

void OrbiterCameraController::setModelParams(const float3& center, float radius) {
    mRotation = float3x3(1.0);
    mModelCenter = center;
    mModelRadius = radius;
    mDistToFocus = radius * 1.1f;
}

}  // namespace Rastery