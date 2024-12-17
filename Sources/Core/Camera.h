#pragma once

#include <memory>

#include "Math.h"
namespace Rastery {

struct CameraData {
    // Camera properties
    float3 posW = float3(0, 0, 2);  ///< World space position.
    float fovY = 60.f;              ///< Field of view in degrees;
    float3 target = float3(0);      ///< lookAt target
    float aspectRatio = 4.f / 3.f;  ///< Aspect ratio(width / height)
    float3 up = float3(0, 1, 0);    ///< Up vector
    float nearZ = 0.01f;            ///< Clip plane near
    float farZ = 10000.f;           ///< Clip plane far

    // Matrices
    float4x4 projViewMat;  ///< mul(projection, view)
    float4x4 projMat;      ///< projection matrix
    float4x4 viewMat;      ///< view matrix
};

class Camera {
   public:
    using SharedPtr = std::shared_ptr<Camera>;
    const CameraData& getData() const { return mData; }

    void setAspectRatio(float value) { mData.aspectRatio = value; }

    void setPositionWorld(float3 v) { mData.posW = v; }
    
    void setUpVec(float3 v) { mData.up = v; }

    void computeCameraData();

    void renderUI();

   private:
    mutable CameraData mData;
};
}  // namespace Rastery