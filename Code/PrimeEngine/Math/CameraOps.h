#ifndef __PYENGINE_2_0_CAMERA_OPS_H__
#define __PYENGINE_2_0_CAMERA_OPS_H__
// These functions abstract low level camera matrix creation

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "PrimeEngine/PrimitiveTypes/PrimitiveTypes.h"
#include "PrimeEngine/Math/Matrix4x4.h"
#include "PrimeEngine/Math/Vector3.h"

namespace CameraOps
{
	Matrix4x4 CreateViewMatrix(Vector3 &pos, Vector3 &target, Vector3 &up);

	Matrix4x4 CreateProjectionMatrix(PrimitiveTypes::Float32 verticalFov, PrimitiveTypes::Float32 aspectRatio,
		PrimitiveTypes::Float32 nearClip, PrimitiveTypes::Float32 farClip);

	Matrix4x4 CreateShadowProjectionMatrix(PrimitiveTypes::Float32 left, PrimitiveTypes::Float32 right, 
										   PrimitiveTypes::Float32 top, PrimitiveTypes::Float32 bottom, 
										   PrimitiveTypes::Float32 nearDistance, PrimitiveTypes::Float32 farDistance);

}; // namespace CameraOps

#endif
