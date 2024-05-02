/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <scene_plugin/api/environment_uid.h>

#include <meta/ext/concrete_base_object.h>

#include "bind_templates.inl"
#include "node_impl.h"

using namespace BASE_NS;
using namespace META_NS;
using namespace CORE_NS;
using namespace SCENE_NS;

namespace {

static constexpr Math::Vec4 ZERO = Math::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
static constexpr Math::Vec4 ONE = Math::Vec4(1.0f, 1.0f, 1.0f, 1.0f);

class EnvImpl : public ConcreteBaseMetaObjectFwd<EnvImpl, NodeImpl, SCENE_NS::ClassId::Environment, IEnvironment> {
    using Super = ConcreteBaseMetaObjectFwd<EnvImpl, NodeImpl, SCENE_NS::ClassId::Environment, IEnvironment>;

    META_IMPLEMENT_INTERFACE_PROPERTY(
        IEnvironment, IEnvironment::BackgroundType, Background, IEnvironment::BackgroundType::NONE)
    META_IMPLEMENT_INTERFACE_PROPERTY(IEnvironment, Math::Vec4, IndirectDiffuseFactor, ONE)
    META_IMPLEMENT_INTERFACE_PROPERTY(IEnvironment, Math::Vec4, IndirectSpecularFactor, ONE)
    META_IMPLEMENT_INTERFACE_PROPERTY(IEnvironment, Math::Vec4, EnvMapFactor, ONE)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IEnvironment, IBitmap::Ptr, RadianceImage, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(IEnvironment, uint32_t, RadianceCubemapMipCount, 0)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IEnvironment, IBitmap::Ptr, EnvironmentImage, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(IEnvironment, float, EnvironmentMapLodLevel, 0.0f)
    META_IMPLEMENT_INTERFACE_ARRAY_PROPERTY(
        IEnvironment, Math::Vec3, IrradianceCoefficients, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(IEnvironment, Math::Quat, EnvironmentRotation, Math::Quat(0.f, 0.f, 0.f, 1.f))
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IEnvironment, Math::Vec4, AdditionalFactor, ZERO)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        IEnvironment, uint64_t, ShaderHandle, CORE_NS::Entity {}.id)

    static constexpr string_view ENV_COMPONENT_NAME = "EnvironmentComponent";
    static constexpr size_t ENV_COMPONENT_NAME_LEN = ENV_COMPONENT_NAME.size() + 1;
    static constexpr string_view ENV_BG = "EnvironmentComponent.background";
    static constexpr string_view ENV_DF = "EnvironmentComponent.indirectDiffuseFactor";
    static constexpr string_view ENV_SF = "EnvironmentComponent.indirectSpecularFactor";
    static constexpr string_view ENV_MF = "EnvironmentComponent.envMapFactor";
    static constexpr string_view ENV_CH = "EnvironmentComponent.radianceCubemap";
    static constexpr string_view ENV_CMC = "EnvironmentComponent.radianceCubemapMipCount";
    static constexpr string_view ENV_MH = "EnvironmentComponent.envMap";
    static constexpr string_view ENV_MLL = "EnvironmentComponent.envMapLodLevel";
    static constexpr string_view ENV_IC = "EnvironmentComponent.irradianceCoefficients";
    static constexpr size_t ENV_IC_SIZE = ENV_IC.size();
    static constexpr string_view ENV_ER = "EnvironmentComponent.environmentRotation";
    static constexpr string_view ENV_AF = "EnvironmentComponent.additionalFactor";
    static constexpr string_view ENV_SH = "EnvironmentComponent.shader";

    bool Build(const IMetadata::Ptr& data) override
    {
        bool ret = false;
        if (ret = Super::Build(data); ret) {
            PropertyNameMask()[ENV_COMPONENT_NAME] = { ENV_BG.substr(ENV_COMPONENT_NAME_LEN),
                ENV_DF.substr(ENV_COMPONENT_NAME_LEN), ENV_SF.substr(ENV_COMPONENT_NAME_LEN),
                ENV_MF.substr(ENV_COMPONENT_NAME_LEN), ENV_CH.substr(ENV_COMPONENT_NAME_LEN),
                ENV_CMC.substr(ENV_COMPONENT_NAME_LEN), ENV_MH.substr(ENV_COMPONENT_NAME_LEN),
                ENV_MLL.substr(ENV_COMPONENT_NAME_LEN), ENV_IC.substr(ENV_COMPONENT_NAME_LEN),
                ENV_ER.substr(ENV_COMPONENT_NAME_LEN), ENV_AF.substr(ENV_COMPONENT_NAME_LEN),
                ENV_SH.substr(ENV_COMPONENT_NAME_LEN) };
            DisableInputHandling();
        }
        return ret;
    }

    bool CompleteInitialization(const string& path) override
    {
        if (!NodeImpl::CompleteInitialization(path)) {
            return false;
        }

        if (auto meta = interface_pointer_cast<IMetadata>(ecsObject_)) {
            ConvertBindChanges<IEnvironment::BackgroundType, uint8_t>(
                propHandler_, META_ACCESS_PROPERTY(Background), meta, ENV_BG);
            BindChanges<Math::Vec4>(propHandler_, META_ACCESS_PROPERTY(IndirectDiffuseFactor), meta, ENV_DF);
            BindChanges<Math::Vec4>(propHandler_, META_ACCESS_PROPERTY(IndirectSpecularFactor), meta, ENV_SF);
            BindChanges<Math::Vec4>(propHandler_, META_ACCESS_PROPERTY(EnvMapFactor), meta, ENV_MF);

            ConvertBindChanges<IBitmap::Ptr, EntityReference, ImageConverter>(
                propHandler_, META_ACCESS_PROPERTY(RadianceImage), meta, ENV_CH);

            ConvertBindChanges<IBitmap::Ptr, EntityReference, ImageConverter>(
                propHandler_, META_ACCESS_PROPERTY(EnvironmentImage), meta, ENV_MH);

            BindChanges<uint32_t>(propHandler_, META_ACCESS_PROPERTY(RadianceCubemapMipCount), meta, ENV_CMC);
            BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(EnvironmentMapLodLevel), meta, ENV_MLL);
            BindChanges<Math::Quat>(propHandler_, META_ACCESS_PROPERTY(EnvironmentRotation), meta, ENV_ER);
            BindChanges<Math::Vec4>(propHandler_, META_ACCESS_PROPERTY(AdditionalFactor), meta, ENV_AF);
            BindChanges<uint64_t>(propHandler_, META_ACCESS_PROPERTY(ShaderHandle), meta, ENV_SH);

            BindChanges<Math::Vec3>(propHandler_, META_ACCESS_PROPERTY(IrradianceCoefficients), meta, ENV_IC);
        }

        return true;
    }

};
} // namespace
SCENE_BEGIN_NAMESPACE()

void RegisterEnvImpl()
{
    auto& registry = GetObjectRegistry();
    registry.RegisterObjectType<EnvImpl>();
}
void UnregisterEnvImpl()
{
    auto& registry = GetObjectRegistry();
    registry.UnregisterObjectType<EnvImpl>();
}
SCENE_END_NAMESPACE()
