
#ifndef SCENE_INTERFACE_ICREATE_MESH_H
#define SCENE_INTERFACE_ICREATE_MESH_H

#include <scene/base/namespace.h>
#include <scene/base/types.h>
#include <scene/interface/intf_mesh.h>

#include <meta/base/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

/// Configuration for creating mesh
struct MeshConfig {
    BASE_NS::string name;
    IMaterial::Ptr material;
};

/// Mesh primitive topology
enum class PrimitiveTopology {
    TRIANGLE_LIST = 3,
    TRIANGLE_STRIP = 4,
};

/// Custom data to create mesh. Normals, uvs and colors are optional.
struct CustomMeshData {
    PrimitiveTopology topology { PrimitiveTopology::TRIANGLE_LIST };
    BASE_NS::vector<BASE_NS::Math::Vec3> vertices;
    BASE_NS::vector<uint32_t> indices;
    BASE_NS::vector<BASE_NS::Math::Vec3> normals;
    BASE_NS::vector<BASE_NS::Math::Vec2> uvs;
    BASE_NS::vector<BASE_NS::Color> colors;
};

/// Interface to create meshes
class ICreateMesh : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ICreateMesh, "727c676f-e20b-403a-93b1-a6116b3b36d9")
public:
    /// Create custom mesh from given data
    virtual Future<IMesh::Ptr> Create(const MeshConfig&, CustomMeshData data) = 0;

    /// Create cube mesh from given data
    virtual Future<IMesh::Ptr> CreateCube(const MeshConfig&, float width, float height, float depth) = 0;
    /// Create plane mesh from given data
    virtual Future<IMesh::Ptr> CreatePlane(const MeshConfig&, float width, float depth) = 0;
    /// Create sphere mesh from given data
    virtual Future<IMesh::Ptr> CreateSphere(const MeshConfig&, float radius, uint32_t rings, uint32_t sectors) = 0;
    /// Create cone mesh from given data
    virtual Future<IMesh::Ptr> CreateCone(const MeshConfig&, float radius, float length, uint32_t sectors) = 0;
};

META_REGISTER_CLASS(MeshCreator, "e920536a-8b5b-4503-a299-7e7f3fc2f603", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::ICreateMesh)
META_TYPE(SCENE_NS::MeshConfig)
META_TYPE(SCENE_NS::PrimitiveTopology)
META_TYPE(SCENE_NS::CustomMeshData)

#endif
