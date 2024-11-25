#include "Camera.h"

#include <imgui.h>
namespace Rastery {
void Camera::computeCameraData() {
    mData.viewMat = lookAt(mData.posW, mData.target, mData.up);
    mData.projMat = perspective(radians(mData.fovY), mData.aspectRatio, mData.nearZ, mData.farZ);
    mData.projViewMat = mData.projMat * mData.viewMat;
}

void Camera::renderUI() {
    ImGui::InputFloat3("Position", (float*)&mData.posW);
    ImGui::InputFloat3("Target", (float*)&mData.target);
    ImGui::SliderFloat("FOV", &mData.fovY, 10.f, 120.f);
}

}  // namespace Rastery
