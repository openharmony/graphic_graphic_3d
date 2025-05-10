
#ifndef SCENE_INTERFACE_IMESH_H
#define SCENE_INTERFACE_IMESH_H

#include <scene/base/namespace.h>
#include <scene/base/types.h>
#include <scene/interface/intf_material.h>

#include <meta/base/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief The IMorpher interface defines the properties of morph targets.
 */
class IMorpher : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMorpher, "a76d0ce9-4d20-4d0d-9c55-32fb703d4766")
public:
    /**
     * @brief Morph target names.
     */
    META_ARRAY_PROPERTY(BASE_NS::string, MorphNames)
    /**
     * @brief Morph target weights.
     */
    META_ARRAY_PROPERTY(float, MorphWeights)
};

/**
 * @brief The ISubMesh interface defines the properties of a SubMesh.
 */
class ISubMesh : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISubMesh, "89060424-5034-43e2-af6c-a7554b90c107")
public:
    /**
     * @brief Material of the object.
     */
    META_PROPERTY(IMaterial::Ptr, Material)
    /**
     * @brief Axis aligned bounding box min of the object.
     */
    META_PROPERTY(BASE_NS::Math::Vec3, AABBMin)
    /**
     * @brief Axis aligned bounding box max of the object.
     */
    META_PROPERTY(BASE_NS::Math::Vec3, AABBMax)
};

/**
 * @brief The IMesh interface defines the properties of a Mesh.
 */
class IMesh : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMesh, "aab724a6-312d-4872-9ff2-b7bed8571b44")
public:
    /**
     * @brief Axis aligned bounding box min. Calculated using all submeshes.
     */
    META_READONLY_PROPERTY(BASE_NS::Math::Vec3, AABBMin)
    /**
     * @brief Axis aligned bounding box max. Calculated using all submeshes.
     */
    META_READONLY_PROPERTY(BASE_NS::Math::Vec3, AABBMax)
    /**
     * @brief Array of submeshes.
     */
    META_READONLY_ARRAY_PROPERTY(ISubMesh::Ptr, SubMeshes)
};

/**
 * @brief Set material for all submeshes
 */
inline bool SetMaterialForAllSubMeshes(const IMesh::Ptr& mesh, const IMaterial::Ptr& material)
{
    if (mesh) {
        for (auto&& s : mesh->SubMeshes()->GetValue()) {
            META_NS::SetValue(s->Material(), material);
        }
        return true;
    }
    return false;
}

/**
 * @brief The IMeshAccess interface defines the methods that can be used to access the Mesh of an object.
 */
class IMeshAccess : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMeshAccess, "63703b44-c1f5-437e-9d57-4248d1b69311")
public:
    /**
     * @brief Sets the mesh of the implementing object.
     */
    virtual Future<bool> SetMesh(const IMesh::Ptr&) = 0;
    /**
     * @brief Returns the mesh of the implementing object.
     */
    virtual Future<IMesh::Ptr> GetMesh() const = 0;
};

/**
 * @brief The IMorphAccess interface defines the methods that can be used to access the morph targets of an object.
 */
class IMorphAccess : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMorphAccess, "df64abcf-fd39-48b9-af1b-74abad800d0c")
public:
    /**
     * @brief IMorpher::Ptr object containing the the morph target information, or null if there are no morph targets.
     */
    META_READONLY_PROPERTY(IMorpher::Ptr, Morpher)
};

META_REGISTER_CLASS(MeshNode, "e0ad59e3-ef66-4b74-8a40-59b1584f14dd", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(SubMesh, "7f48b697-9fba-43da-8b15-f61fbbf2a925", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(Mesh, "8478fc2b-13fe-4b59-963a-370c04a94d15", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IMorpher)
META_INTERFACE_TYPE(SCENE_NS::ISubMesh)
META_INTERFACE_TYPE(SCENE_NS::IMesh)

#endif
