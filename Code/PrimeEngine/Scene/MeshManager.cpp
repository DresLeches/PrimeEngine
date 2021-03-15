// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "MeshManager.h"
// Outer-Engine includes

// Inter-Engine includes
#include "PrimeEngine/FileSystem/FileReader.h"
#include "PrimeEngine/APIAbstraction/GPUMaterial/GPUMaterialSet.h"
#include "PrimeEngine/PrimitiveTypes/PrimitiveTypes.h"
#include "PrimeEngine/APIAbstraction/Texture/Texture.h"
#include "PrimeEngine/APIAbstraction/Effect/EffectManager.h"
#include "PrimeEngine/APIAbstraction/GPUBuffers/VertexBufferGPUManager.h"
#include "PrimeEngine/../../GlobalConfig/GlobalConfig.h"
#include "PrimeEngine/Scene/RootSceneNode.h"

#include "PrimeEngine/Geometry/SkeletonCPU/SkeletonCPU.h"

#include "PrimeEngine/Scene/RootSceneNode.h"

#include "Light.h"

// Sibling/Children includes

#include "MeshInstance.h"
#include "Skeleton.h"
#include "SceneNode.h"
#include "DrawList.h"
#include "SH_DRAW.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"

namespace PE {
namespace Components{

PE_IMPLEMENT_CLASS1(MeshManager, Component);
MeshManager::MeshManager(PE::GameContext &context, PE::MemoryArena arena, Handle hMyself)
	: Component(context, arena, hMyself)
	, m_assets(context, arena, 256)
{
}

PE::Handle MeshManager::getAsset(const char *asset, const char *package, int &threadOwnershipMask)
{
	char key[StrTPair<Handle>::StrSize];
	sprintf(key, "%s/%s", package, asset);
	
	int index = m_assets.findIndex(key);
	if (index != -1)
	{
		return m_assets.m_pairs[index].m_value;
	}
	Handle h;

	if (StringOps::endswith(asset, "skela"))
	{
		PE::Handle hSkeleton("Skeleton", sizeof(Skeleton));
		Skeleton *pSkeleton = new(hSkeleton) Skeleton(*m_pContext, m_arena, hSkeleton);
		pSkeleton->addDefaultComponents();

		pSkeleton->initFromFiles(asset, package, threadOwnershipMask);
		h = hSkeleton;
	}
	else if (StringOps::endswith(asset, "mesha"))
	{
		MeshCPU mcpu(*m_pContext, m_arena);
		mcpu.ReadMesh(asset, package, "");
		
		PE::Handle hMesh("Mesh", sizeof(Mesh));
		Mesh *pMesh = new(hMesh) Mesh(*m_pContext, m_arena, hMesh);
		pMesh->addDefaultComponents();

		pMesh->loadFromMeshCPU_needsRC(mcpu, threadOwnershipMask);

#if PE_API_IS_D3D11
		// todo: work out how lods will work
		//scpu.buildLod();
#endif
        // generate collision volume here. or you could generate it in MeshCPU::ReadMesh()
        pMesh->m_performBoundingVolumeCulling = true; // will now perform tests for this mesh

		// Find the number of coordinates stored in the position buffer for the mesh
        PrimitiveTypes::UInt32 sizeOfBuffer = pMesh->m_hPositionBufferCPU.getObject<PositionBufferCPU>()->m_values.m_size;

		// Verify if the static mesh has vertex data or not
		if (sizeOfBuffer >= 3)
		{
			PrimitiveTypes::Float32 minX = pMesh->m_hPositionBufferCPU.getObject<PositionBufferCPU>()->m_values[0];
			PrimitiveTypes::Float32 maxX = pMesh->m_hPositionBufferCPU.getObject<PositionBufferCPU>()->m_values[0];

			PrimitiveTypes::Float32 minY = pMesh->m_hPositionBufferCPU.getObject<PositionBufferCPU>()->m_values[1];
			PrimitiveTypes::Float32 maxY = pMesh->m_hPositionBufferCPU.getObject<PositionBufferCPU>()->m_values[1];

			PrimitiveTypes::Float32 minZ = pMesh->m_hPositionBufferCPU.getObject<PositionBufferCPU>()->m_values[2];
			PrimitiveTypes::Float32 maxZ = pMesh->m_hPositionBufferCPU.getObject<PositionBufferCPU>()->m_values[2];

			// Find min-max values of the AABB for this mesh
			for (PrimitiveTypes::UInt32 i = 0; i < sizeOfBuffer; i += 3)
			{
				// Find min-max of X coordinates
				if (minX > pMesh->m_hPositionBufferCPU.getObject<PositionBufferCPU>()->m_values[i])
				{
					minX = pMesh->m_hPositionBufferCPU.getObject<PositionBufferCPU>()->m_values[i];
				}

				if (maxX < pMesh->m_hPositionBufferCPU.getObject<PositionBufferCPU>()->m_values[i])
				{
					maxX = pMesh->m_hPositionBufferCPU.getObject<PositionBufferCPU>()->m_values[i];
				}

				// Find min-max of Y coordinates
				if (minY > pMesh->m_hPositionBufferCPU.getObject<PositionBufferCPU>()->m_values[i + 1])
				{
					minY = pMesh->m_hPositionBufferCPU.getObject<PositionBufferCPU>()->m_values[i + 1];
				}

				if (maxY < pMesh->m_hPositionBufferCPU.getObject<PositionBufferCPU>()->m_values[i + 1])
				{
					maxY = pMesh->m_hPositionBufferCPU.getObject<PositionBufferCPU>()->m_values[i + 1];
				}

				// Find min-max of Z coordinates
				if (minZ > pMesh->m_hPositionBufferCPU.getObject<PositionBufferCPU>()->m_values[i + 2])
				{
					minZ = pMesh->m_hPositionBufferCPU.getObject<PositionBufferCPU>()->m_values[i + 2];
				}

				if (maxZ < pMesh->m_hPositionBufferCPU.getObject<PositionBufferCPU>()->m_values[i + 2])
				{
					maxZ = pMesh->m_hPositionBufferCPU.getObject<PositionBufferCPU>()->m_values[i + 2];
				}
			}

			// Assign AABB to the mesh object
			pMesh->m_AABB.minX = minX;
			pMesh->m_AABB.maxX = maxX;

			pMesh->m_AABB.minY = minY;
			pMesh->m_AABB.maxY = maxY;

			pMesh->m_AABB.minZ = minZ;
			pMesh->m_AABB.maxZ = maxZ;
		}

		/*for (int iInst = 0; iInst < pMesh->m_instances.m_size; ++iInst) {
			MeshInstance *pInst = pMesh->m_instances[iInst].getObject<MeshInstance>();
			if (pInst->getFirstParentByType<PE::Components::SceneNode>().getObject<SceneNode>() != nullptr) {
				pInst->getFirstParentByType<PE::Components::SceneNode>().getObject<SceneNode>()->m_AABB = pMesh->m_AABB;
				RootSceneNode::Instance()->m_meshSNs.push_back(pInst->getFirstParentByType<PE::Components::SceneNode>().getObject<SceneNode>());
			} 
		}*/

		h = hMesh;
	}


	PEASSERT(h.isValid(), "Something must need to be loaded here");

	RootSceneNode::Instance()->addComponent(h);
	m_assets.add(key, h);
	return h;
}

void MeshManager::registerAsset(const PE::Handle &h)
{
	static int uniqueId = 0;
	++uniqueId;
	char key[StrTPair<Handle>::StrSize];
	sprintf(key, "__generated_%d", uniqueId);
	
	int index = m_assets.findIndex(key);
	PEASSERT(index == -1, "Generated meshes have to be unique");
	
	RootSceneNode::Instance()->addComponent(h);
	m_assets.add(key, h);
}

}; // namespace Components
}; // namespace PE
