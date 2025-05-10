
#ifndef SCENE_API_SCENE_H
#define SCENE_API_SCENE_H

#include <scene/api/node.h>
#include <scene/api/resource.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/interface/intf_image.h>
#include <scene/interface/intf_light.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>
#include <scene/interface/resource/intf_render_resource_manager.h>

#include <meta/api/animation.h>
#include <meta/api/iteration.h>

SCENE_BEGIN_NAMESPACE()

class Scene;

#define SCENE_RESOURCE_FACTORY_CREATE_NODE(NodeName, NodeType, ClassId) \
    template<const AsyncCallType& CallType = Sync>                      \
    auto Create##NodeName(BASE_NS::string_view path)                    \
    {                                                                   \
        return CallCreateNode<NodeType, CallType>(path, ClassId);       \
    }

/**
 * @brief The SceneResourceParameters class defines generic parameters for resource creation.
 */
struct SceneResourceParameters {
    SceneResourceParameters() = default;
    SceneResourceParameters(BASE_NS::string_view path) : path(path) {}
    SceneResourceParameters(const char* path) : path(path) {}
    /// Path to load the resource from. Depending on resource type this parameter may be ignored.
    BASE_NS::string path;
};

/**
 * @brief The MaterialResourceParameters class defines Material creation specific parameters.
 */
struct MaterialResourceParameters : public SceneResourceParameters {
    /// Type of the material
    MaterialType type { MaterialType::METALLIC_ROUGHNESS };
};

/**
 * @brief The TextResourceParameters class defines Text3D node creation specific parameters.
 */
struct TextResourceParameters : public SceneResourceParameters {
    /// The text string to show
    BASE_NS::string text;
    /// Font to use
    BASE_NS::string font;
    /// Height of the rasterized 2d texture.
    float fontSize {};
    /// Text color in SRGB RGBA.
    BASE_NS::Math::Vec4 color;
};

/**
 * @brief RenderContextResourceFactory can be used to instantiate new RenderContext specific resources (shared between
 *        all scenes using the same RenderContext).
 */
class RenderContextResourceFactory {
public:
    RenderContextResourceFactory() = delete;
    RenderContextResourceFactory(const IScene::Ptr& scene) : scene_(scene) {};
    virtual ~RenderContextResourceFactory() = default;
    META_DEFAULT_COPY_MOVE(RenderContextResourceFactory)
    operator bool() const noexcept
    {
        return scene_.lock() != nullptr;
    }
    /// Creates a new environment with given parameters.
    META_API_ASYNC auto CreateEnvironment(const SceneResourceParameters& params)
    {
        return CallCreateResource<Environment, CallType>(params, ClassId::Environment);
    }
    /// Creates a new material with given parameters.
    META_API_ASYNC auto CreateMaterial(const MaterialResourceParameters& params)
    {
        auto f = CallScene([&](auto& scene) {
            return scene.CreateObject(ClassId::Material).Then([params](const META_NS::IObject::Ptr& object) {
                auto material = interface_pointer_cast<IMaterial>(object);
                if (material) {
                    SetValue(material->Type(), params.type);
                }
                return material;
            });
        });
        return Internal::UnwrapFuture<CallType, Material>(BASE_NS::move(f));
    }
    /// Creates a new mesh with given parameters.
    META_API_ASYNC auto CreateMesh(const SceneResourceParameters& params)
    {
        return CallCreateResource<Mesh, CallType>(params, ClassId::Mesh);
    }
    /// Creates a new shader with given parameters.
    META_API_ASYNC auto CreateShader(const SceneResourceParameters& params)
    {
        auto f = CallScene([&](auto& scene) {
            return scene.template CreateObject<IRenderResourceManager>(ClassId::RenderResourceManager)
                .Then([params](const IRenderResourceManager::Ptr& manager) {
                    return manager ? manager->LoadShader(params.path).GetResult() : IShader::Ptr {};
                });
        });
        return Internal::UnwrapFuture<CallType, Shader>(BASE_NS::move(f));
    }
    /// Creates a new image with given parameters.
    META_API_ASYNC auto CreateImage(const SceneResourceParameters& params)
    {
        auto f = CallScene([&](auto& scene) {
            return scene.template CreateObject<IRenderResourceManager>(ClassId::RenderResourceManager)
                .Then([params](const IRenderResourceManager::Ptr& manager) {
                    return manager ? manager->LoadImage(params.path).GetResult() : IImage::Ptr {};
                });
        });
        return Internal::UnwrapFuture<CallType, Image>(BASE_NS::move(f));
    }

protected:
    template<class Type, const META_NS::AsyncCallType& CallType>
    auto CallCreateResource(const SceneResourceParameters& params, META_NS::ObjectId id)
    {
        auto f = CallScene([&](auto& scene) { return scene.CreateObject(id); });
        return Internal::UnwrapFuture<CallType, Type>(BASE_NS::move(f));
    }
    template<typename Fn>
    auto CallScene(Fn&& fn) const
    {
        auto scene = scene_.lock();
        return scene ? fn(*scene) : decltype(fn(*scene)) {};
    }

private:
    IScene::WeakPtr scene_;
};

/**
 * @brief SceneResourceFactory can be used to instantiate new Scene specific resources (unique to each Scene instance).
 */
class SceneResourceFactory : public RenderContextResourceFactory {
public:
    SceneResourceFactory() = delete;
    SceneResourceFactory(const IScene::Ptr& scene) : RenderContextResourceFactory(scene) {};
    virtual ~SceneResourceFactory() = default;
    META_DEFAULT_COPY_MOVE(SceneResourceFactory)
    /// Creates a new empty node with given parameters.
    META_API_ASYNC auto CreateNode(const SceneResourceParameters& params)
    {
        return CallCreateNode<Node, CallType>(params, ClassId::Node);
    }
    /// Creates a new geometry node with given parameters.
    META_API_ASYNC auto CreateGeometryNode(const SceneResourceParameters& params, const IMesh::Ptr& mesh)
    {
        auto f = CallScene([&](auto& scene) {
            return scene.template CreateNode<IMeshAccess>(params.path, ClassId::MeshNode)
                .Then([mesh](const IMeshAccess::Ptr& node) {
                    bool set = !mesh || node->SetMesh(mesh).GetResult();
                    return set ? interface_pointer_cast<INode>(node) : nullptr;
                });
        });
        return Internal::UnwrapFuture<CallType, Geometry>(BASE_NS::move(f));
    }
    /// Creates a new camera node with given parameters.
    META_API_ASYNC auto CreateCameraNode(const SceneResourceParameters& params)
    {
        return CallCreateNode<Camera, CallType>(params, ClassId::CameraNode);
    }
    /// Creates a new light node with given parameters.
    META_API_ASYNC auto CreateLightNode(const SceneResourceParameters& params)
    {
        return CallCreateNode<Light, CallType>(params, ClassId::LightNode);
    }
    /// Creates a new text node with given parameters.
    META_API_ASYNC auto CreateTextNode(const TextResourceParameters& params)
    {
        return CallCreateNode<Text3D, CallType>(params, ClassId::TextNode);
    }

private:
    template<class Type, const META_NS::AsyncCallType& CallType>
    auto CallCreateNode(const SceneResourceParameters& params, META_NS::ObjectId id)
    {
        auto f = CallScene([&](auto& scene) { return scene.CreateNode(params.path, id); });
        return Internal::UnwrapFuture<CallType, Type>(BASE_NS::move(f));
    }
};

/**
 * @brief The Component class is a wrapper for IComponent::Ptr.
 *        Components can be queried from Nodes by calling Scene.GetComponent() or created
 *        at runtime by calling Scene.CreateComponent().
 * @note  The Component's metadata contains all of the component specific properties.
 */
class Component : public META_NS::Object {
public:
    META_INTERFACE_OBJECT(Component, META_NS::Object, IComponent)

    /**
     * @brief Returns a component property with given name.
     * @param name Name of the component property to return.
     */
    template<typename Type>
    auto GetProperty(BASE_NS::string_view name) const
    {
        auto meta = META_NS::Metadata(*this);
        auto p = meta.GetProperty<Type>(name);
        // Could also be "ComponentName.PropertyName"
        return p ? p : meta.GetProperty<Type>(GetName() + "." + name);
    }
    /**
     * @brief Returns a component array property with given name.
     * @note  Equivalent to calling component.GetProperty<BASE_NS::vector<Type>>(name)
     * @param name Name of the component property to return.
     */
    template<typename Type>
    auto GetArrayProperty(BASE_NS::string_view name) const
    {
        return GetProperty<BASE_NS::vector<Type>>(name);
    }

private:
    auto& InitializeComponent()
    {
        META_INTERFACE_OBJECT_CALL_PTR(PopulateAllProperties());
        return *this;
    }
    friend class Scene;
};

/// Wrapper for scene objects which implement SCENE_NS::IScene.
class Scene : public META_NS::Object {
public:
    META_INTERFACE_OBJECT(Scene, META_NS::Object, IScene)
    /// Initialize a Scene object from a Node.
    explicit Scene(const Node& node) : META_NS::Object(node.GetScene()) {}
    /// Returns a resource factory for creating resources associated with this scene.
    auto GetResourceFactory()
    {
        return SceneResourceFactory(GetPtr<IScene>());
    }
    /// @see IScene::GetRootNode
    META_API_ASYNC auto GetRootNode() const
    {
        return META_INTERFACE_OBJECT_ASYNC_CALL_PTR(Node, GetRootNode());
    }
    /// @see IScene::FindNode
    META_API_ASYNC auto GetNode(BASE_NS::string_view path, META_NS::ObjectId id = {}) const
    {
        return META_INTERFACE_OBJECT_ASYNC_CALL_PTR(Node, FindNode(path, id));
    }
    /**
     * @brief Removes the node (and its children) from the scene.
     * @see IScene::RemoveNode
     * @param node The node to remove. The Node object is unitinialized as a result of this call.
     */
    META_API_ASYNC auto RemoveNode(Node& node)
    {
        auto ret = META_INTERFACE_OBJECT_ASYNC_CALL_PTR(bool, RemoveNode(node.GetPtr<INode>()));
        node.Release();
        return ret;
    }
    /// @see IScene::GetCaemras
    META_API_ASYNC auto GetCameras() const
    {
        return META_INTERFACE_OBJECT_ASYNC_CALL_PTR(META_NS::Internal::ArrayCast<Camera>, GetCameras());
    }
    /// @see IScene::GetAnimations
    META_API_ASYNC auto GetAnimations() const
    {
        return META_INTERFACE_OBJECT_ASYNC_CALL_PTR(META_NS::Internal::ArrayCast<META_NS::Animation>, GetAnimations());
    }
    /// @see IScene::SetRenderMode
    META_API_ASYNC auto SetRenderMode(RenderMode mode)
    {
        return META_INTERFACE_OBJECT_ASYNC_CALL_PTR(bool, SetRenderMode(BASE_NS::move(mode)));
    }
    /// @see IScene::GetRenderMode
    META_API_ASYNC auto GetRenderMode() const
    {
        return META_INTERFACE_OBJECT_ASYNC_CALL_PTR(bool, GetRenderMode());
    }
    /**
     * @brief Returns an underlying ECS component from a Node with given name.
     * @param node The Node to query from.
     * @param name The component name to query.
     * @return a Component with given name, if one exists in the target Node.
     */
    auto GetComponent(const Node& node, BASE_NS::string_view name) const
    {
        auto component = Component(META_INTERFACE_OBJECT_CALL_PTR(GetComponent(node, name)));
        component.InitializeComponent();
        return component;
    }
    /**
     * @brief Creates an underlying ECS component with given component manager name to the given node.
     * @note If there is already a component with given name associated with the node, the existing component instance
     *       is returned.
     * @param node The node to create the component to.
     * @param name Name of the ECS component manager to instantiate. E.g. "CameraComponent" would create a camera
     *        component and associate it with the node.
     * @return The component with given component manager name.
     */
    META_API_ASYNC auto CreateComponent(const Node& node, BASE_NS::string_view name) const
    {
        auto f = META_INTERFACE_OBJECT_CALL_PTR(CreateComponent(node, name)).Then([](const IComponent::Ptr& component) {
            if (component) {
                component->PopulateAllProperties();
            }
            return component;
        });
        return Internal::UnwrapFuture<CallType, Component>(BASE_NS::move(f));
    }
    /**
     * @brief Imports a Scene into this scene under given node.
     * @param scene The scene to import.
     * @param node The node under which the scene should be imported. If null, the imported node is placed under root
     *        node of this scene.
     * @param name The name which should be given to the root node of the imported scene, placed as a child of node.
     */
    META_API_ASYNC auto ImportScene(const IScene::Ptr& scene, const INode::Ptr& node, BASE_NS::string_view name)
    {
        Future<INode::Ptr> f;
        if (auto import = interface_cast<INodeImport>(node)) {
            f = import->ImportChildScene(scene, name);
        } else {
            f = GetRootNode<META_NS::Async>().Then([scene, name = BASE_NS::string(name)](const INode::Ptr& root) {
                auto import = interface_cast<INodeImport>(root);
                return import ? import->ImportChildScene(scene, name).GetResult() : nullptr;
            });
        }
        return Internal::UnwrapFuture<CallType, Node>(BASE_NS::move(f));
    }
    /**
     * @brief Imports a Scene from a resource into this scene under given node.
     * @param uri Uri of the scene resource to import (.gltf or .scene)
     * @param node The node under which the scene should be imported. If null, the imported node is placed under root
     *        node of this scene.
     * @param name The name which should be given to the root node of the imported scene, placed as a child of node.
     */
    META_API_ASYNC auto ImportScene(BASE_NS::string_view uri, const INode::Ptr& node, BASE_NS::string_view name)
    {
        Future<INode::Ptr> f;
        if (auto import = interface_cast<INodeImport>(node)) {
            f = import->ImportChildScene(uri, name);
        } else {
            f = GetRootNode<META_NS::Async>().Then(
                [uri = BASE_NS::string(uri), name = BASE_NS::string(name)](const INode::Ptr& root) {
                    auto import = interface_cast<INodeImport>(root);
                    return import ? import->ImportChildScene(uri, name).GetResult() : nullptr;
                });
        }
        return Internal::UnwrapFuture<CallType, Node>(BASE_NS::move(f));
    }
    /**
     * @brief Finds the first occurrence of a node with given name in the scene.
     * @note  If the full path is known, it is more efficient to call Scene::GetNode
     * @param name Name of the node to find.
     * @param traversalType Can be used to control how the scene is traversed, or to limit finding only immediate
     *        children by specifying TraversalType::NO_HIERARCHY.
     * @return The first node matching the parameters or an invalid node in case of failure.
     */
    META_API_ASYNC auto FindNode(
        BASE_NS::string_view name, META_NS::TraversalType traversalType = META_NS::TraversalType::FULL_HIERARCHY) const
    {
        auto f = META_INTERFACE_OBJECT_CALL_PTR(GetRootNode())
                     .Then([name = BASE_NS::string(name), traversalType](const INode::Ptr& node) -> INode::Ptr {
                         return Node(Internal::FindNode(node, name, traversalType));
                     });
        return Internal::UnwrapFuture<CallType, Node>(BASE_NS::move(f));
    }
    /**
     * @brief Finds an animation from the scene.
     * @param name Name of the animation to find.
     * @return The animation with given name or an invalid object in case of failure.
     */
    META_API_ASYNC auto FindAnimation(BASE_NS::string_view name)
    {
        auto f =
            META_INTERFACE_OBJECT_CALL_PTR(GetAnimations())
                .Then([name = BASE_NS::string(name)](
                          const BASE_NS::vector<META_NS::IAnimation::Ptr>& animations) -> META_NS::IAnimation::Ptr {
                    return Internal::FindFromContainer<META_NS::IAnimation>(animations, name);
                });
        return Internal::UnwrapFuture<CallType, META_NS::Animation>(BASE_NS::move(f));
    }
};

SCENE_END_NAMESPACE()

#endif // SCENE_API_SCENE_H
