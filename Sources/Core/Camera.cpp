#include "Camera.h"

namespace Rastery {
void Camera::computeCameraData() {
    mData.viewMat = lookAt(mData.posW, mData.target, mData.up);
    mData.projMat = perspective(radians(mData.fovY), mData.aspectRatio, mData.nearZ, mData.farZ);
    mData.projViewMat = mData.projMat * mData.viewMat;
}

}  // namespace Rastery
