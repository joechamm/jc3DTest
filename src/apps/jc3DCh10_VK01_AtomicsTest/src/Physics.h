#ifndef __PHYSICS_H__
#define __PHYSICS_H__

#include <cstdlib>
#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
using glm::mat4;
using glm::vec4;
using glm::vec3;
using glm::vec2;

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4305)
#endif

#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btAlignedObjectArray.h"
#include "BulletCollision/NarrowPhaseCollision/btRaycastCallback.h"

#if defined(_MSC_VER)
#pragma warning(pop)
#endif 

#include <jcCommonFramework/UtilsMath.h>

struct Physics
{
	std::vector<mat4> boxTransform_;

private:
	std::vector<std::unique_ptr<btRigidBody>> rigidBodies_;
	
	btDefaultCollisionConfiguration collisionConfiguration_;
	btCollisionDispatcher collisionDispatcher_;
	btDbvtBroadphase broadphase_;
	btSequentialImpulseConstraintSolver solver_;
	btDiscreteDynamicsWorld dynamicsWorld_;

public:
	Physics ()
		: collisionDispatcher_ ( &collisionConfiguration_ )
		, dynamicsWorld_ ( &collisionDispatcher_, &broadphase_, &solver_, &collisionConfiguration_ )
	{
		dynamicsWorld_.setGravity ( btVector3 ( 0, -9.8f, 0 ) );

		// add "floor" object - large massless box
		addBox ( vec3 ( 100.0f, 0.05f, 100.0f ), btQuaternion ( 0, 0, 0, 1 ), vec3 ( 0.0f, 0.0f, 0.0f ), 0.0f );
	}

	void addBox ( const vec3& halfSize, const btQuaternion& orientation, const vec3& position, float mass );

	void update ( float deltaSeconds );
};



#endif // !__PHYSICS_H__
