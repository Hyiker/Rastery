#pragma once
#include <memory>

#include "Camera.h"
#include "Macros.h"
#include "Math.h"
#include "Window.h"

namespace Rastery {

class RASTERY_API OrbiterCameraController {
   public:
    using SharedPtr = std::shared_ptr<OrbiterCameraController>;
    OrbiterCameraController(const Camera::SharedPtr& pCam);

    void onMouseEvent(const MouseEvent& mouseEvent);

    /** Update camera data.
     */
    void update();

    void setModelParams(const float3& center, float radius);

   private:
    Camera::SharedPtr mpCamera;

    float3x3 mRotation;  ///< Current rotation from kToCameraBase
    float3 mLastVec;     ///< left button down position vector

    float mDistToFocus;
    float mModelRadius;
    float3 mModelCenter = float3(0);

    bool mIsLeftButtonDown = false;
};
}  // namespace Rastery