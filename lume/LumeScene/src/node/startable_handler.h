
#ifndef SCENE_SRC_STARTABLE_HANDLER_H
#define SCENE_SRC_STARTABLE_HANDLER_H

#include <scene/ext/intf_internal_scene.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_scene.h>

SCENE_BEGIN_NAMESPACE()

class StartableHandler {
public:
    enum class StartType : uint8_t {
        DEFERRED = 0,
        DIRECT = 1,
    };

    StartableHandler() = delete;
    StartableHandler(const INode::Ptr& node) : node_(node) {}
    virtual ~StartableHandler() = default;
    bool Start(const META_NS::IObject::Ptr& attachment, StartType type);
    bool Stop(const META_NS::IObject::Ptr& attachment);
    bool StartAll(StartType type);
    bool StopAll();

private:
    IInternalScene::Ptr GetScene() const;
    INode::WeakPtr node_;
};

SCENE_END_NAMESPACE()

#endif // SCENE_SRC_STARTABLE_HANDLER_H
