#ifndef __PrimeEngineCollision__
#define __PrimeEngineCollision__
#include "PrimeEngine/Math/Vector3.h"

namespace PE
{
namespace Components
{

struct AABB
{
	AABB();
	AABB(Vector3 min, Vector3 max);

	Vector3 m_min;
	Vector3 m_max;
};

struct OBB
{
	Vector3 m_center;			// center of oriented bounding boxes
	Vector3 m_localAxes[3];		// local axes
	Vector3 m_extent;			// halfwidth extent of OBB
};



bool TestAABB_AABB(AABB& a, AABB& b);



}	// Components

}	// PE


#endif