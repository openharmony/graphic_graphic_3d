#ifndef OHOS_RENDER_3D_SR_MODULE_H
#define OHOS_RENDER_3D_SR_MODULE_H

#include <core/intf_engine.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <core/engine_info.h>
#include <core/implementation_uids.h>
#include <core/io/intf_file_manager.h>
#include <core/namespace.h>
#include <core/os/intf_platform.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/property/intf_property_handle.h>

#include <scene/base/namespace.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_material.h>

#include <3d/intf_graphics_context.h>

#include <plugin_sr/implementation_uids.h>
#include <plugin_sr/render/intf_render_data_store_sr.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <string>
#include <string_view>

namespace OHOS::Render3D {
constexpr BASE_NS::string_view RENDER_DATA_STORE_SR_NAME { "RenderDataStoreSR" };

enum class MethodTypeSR {
    BILINEAR = 0,
    LUT,
    HGSR1,
    COUNT
};

enum class QualityTypeSR {
    LOW = 0,
    BALANCED,
    HIGH,
    ULTRA
};

struct SRData {
    bool enable_ = false;
    MethodTypeSR type_ { MethodTypeSR::BILINEAR };
    float rate_ = 1.0;
    int width_ = -1;
    int height_ = -1;
};

class SRModule {
public:
    SRModule();
    void Init(BASE_NS::shared_ptr<SCENE_NS::IInternalScene> scene,
        BASE_NS::shared_ptr<RENDER_NS::IRenderContext> renderContext,
        BASE_NS::shared_ptr<CORE_NS::IEngine> engine,
        BASE_NS::refcnt_ptr<CORE_NS::IEcs> ecs);

    void Update(BASE_NS::shared_ptr<SCENE_NS::IScene> scene);

    static bool EnableSR();
    static const SRData InitConfig(BASE_NS::shared_ptr<SCENE_NS::IInternalScene> scene,
        BASE_NS::shared_ptr<RENDER_NS::IRenderContext> renderContext);
    static const SRData Getconfig();
    static void SetWindowSize(int width, int height);
    static void Enable();
    
    static RENDER_NS::RenderHandleReference CreateGpuResource(
        BASE_NS::shared_ptr<RENDER_NS::IRenderContext> rc, float width, float height);
    static void AttachComponent();
    inline static bool srInitialized_ = false;

private:
    static SRData sr_;
    static BASE_NS::refcnt_ptr<CORE_NS::IEcs> ecs_;
    static CORE_NS::EntityReference srConfigEntity_;
};
} // namespace OHOS::Render3D