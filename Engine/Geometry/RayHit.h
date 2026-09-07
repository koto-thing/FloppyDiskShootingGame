#pragma once
#include "../Math/Vector3.h"
/** @brief レイと形状の交差結果 */
struct RayHit { float distance = 0.0f; Vector3 point = Vector3::Zero; Vector3 normal = Vector3::Zero; };
