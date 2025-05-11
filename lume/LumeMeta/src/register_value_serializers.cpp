#include <base/math/matrix.h>
#include <base/math/quaternion.h>
#include <base/math/vector.h>

#include <meta/base/ids.h>
#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>
#include <meta/base/time_span.h>
#include <meta/ext/serialization/common_value_serializers.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/intf_iterable.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_startable.h>

META_BEGIN_NAMESPACE()

namespace Internal {

// workaround for VC bug not being able to handle decltype(out.x) in lambda below
template<typename T>
static ISerNode::Ptr ExportVector(IExportFunctions& f, const T* v, size_t size)
{
    return f.ExportToNode(ArrayAny<T>(BASE_NS::array_view<const T>(v, v + size)));
}

// workaround for VC bug not being able to handle decltype(out.x) in lambda below
template<typename T, size_t Size>
static bool ImportVector(IImportFunctions& f, const ISerNode::ConstPtr& node, T (&out)[Size])
{
    ArrayAny<T> arr;
    if (f.ImportFromNode(node, arr)) {
        const auto& vec = arr.InternalGetValue();
        if (vec.size() == Size) {
            for (size_t i = 0; i != Size; ++i) {
                out[i] = vec[i];
            }
            return true;
        }
    }
    return false;
};

void RegisterValueSerializers(IObjectRegistry& registry)
{
    auto vecExport = [](IExportFunctions& f, const auto& v) {
        constexpr size_t size = sizeof(v.data) / sizeof(v.data[0]); /*NOLINT(bugprone-sizeof-expression)*/
        return ExportVector(f, v.data, size);
    };
    auto vecImport = [](IImportFunctions& f, const ISerNode::ConstPtr& node, auto& out) {
        return ImportVector(f, node, out.data);
    };
    auto matExport = [](IExportFunctions& f, const auto& v) {
        constexpr size_t size = sizeof(v.base) / sizeof(v.base[0]); /*NOLINT(bugprone-sizeof-expression)*/
        return ExportVector(f, v.base, size);
    };
    auto matImport = [](IImportFunctions& f, const ISerNode::ConstPtr& node, auto& out) {
        return ImportVector(f, node, out.base);
    };
    auto enumExport = [](IExportFunctions& f, const auto& v) { return EnumExport(f, v); };
    auto enumImport = [](IImportFunctions& f, const ISerNode::ConstPtr& node, auto& out) {
        return EnumImport(f, node, out);
    };

    auto& data = registry.GetGlobalSerializationData();
    RegisterSerializer<BASE_NS::Math::Vec2>(data, vecExport, vecImport);
    RegisterSerializer<BASE_NS::Math::Vec3>(data, vecExport, vecImport);
    RegisterSerializer<BASE_NS::Math::Vec4>(data, vecExport, vecImport);
    RegisterSerializer<BASE_NS::Math::UVec2>(data, vecExport, vecImport);
    RegisterSerializer<BASE_NS::Math::UVec3>(data, vecExport, vecImport);
    RegisterSerializer<BASE_NS::Math::UVec4>(data, vecExport, vecImport);
    RegisterSerializer<BASE_NS::Math::IVec2>(data, vecExport, vecImport);
    RegisterSerializer<BASE_NS::Math::IVec3>(data, vecExport, vecImport);
    RegisterSerializer<BASE_NS::Math::IVec4>(data, vecExport, vecImport);
    RegisterSerializer<BASE_NS::Math::Quat>(data, vecExport, vecImport);
    RegisterSerializer<BASE_NS::Math::Mat3X3>(data, matExport, matImport);
    RegisterSerializer<BASE_NS::Math::Mat4X4>(data, matExport, matImport);

    auto colorExport = [](IExportFunctions& f, const auto& v) {
        constexpr size_t size = sizeof(v.data) / sizeof(v.data[0]); /*NOLINT(bugprone-sizeof-expression)*/
        return ExportVector(f, v.data, size);
    };
    auto colorImport = [](IImportFunctions& f, const ISerNode::ConstPtr& node, auto& out) {
        return ImportVector(f, node, out.data);
    };

    RegisterSerializer<BASE_NS::Color>(data, colorExport, colorImport);

    auto idExport = [](IExportFunctions& f, const auto& v) {
        return f.ExportToNode(Any<BASE_NS::string>(v.ToString()));
    };
    auto idImport = [](IImportFunctions& f, const ISerNode::ConstPtr& node, auto& out) {
        Any<BASE_NS::string> any;
        if (f.ImportFromNode(node, any)) {
            out = BASE_NS::StringToUid(any.InternalGetValue());
            return true;
        }
        return false;
    };

    RegisterSerializer<TypeId>(data, idExport, idImport);
    RegisterSerializer<ObjectId>(data, idExport, idImport);
    RegisterSerializer<InstanceId>(data, idExport, idImport);

    RegisterSerializer<BASE_NS::Uid>(
        data,
        [](IExportFunctions& f, const auto& v) {
            return f.ExportToNode(Any<BASE_NS::string>(BASE_NS::to_string(v).c_str()));
        },
        idImport);

    RegisterSerializer<TimeSpan>(
        data, [](IExportFunctions& f, const auto& v) { return f.ExportToNode(Any<int64_t>(v.ToMicroseconds())); },
        [](IImportFunctions& f, const ISerNode::ConstPtr& node, auto& out) {
            Any<int64_t> any;
            if (f.ImportFromNode(node, any)) {
                out = TimeSpan::Microseconds(any.InternalGetValue());
                return true;
            }
            return false;
        });

    RegisterSerializer<StartableState>(data, enumExport, enumImport);
    RegisterSerializer<StartBehavior>(data, enumExport, enumImport);
    RegisterSerializer<TraversalType>(data, enumExport, enumImport);
}

void UnRegisterValueSerializers(IObjectRegistry& registry)
{
    auto& data = registry.GetGlobalSerializationData();
    UnregisterSerializer<TraversalType>(data);
    UnregisterSerializer<StartBehavior>(data);
    UnregisterSerializer<StartableState>(data);
    UnregisterSerializer<TimeSpan>(data);
    UnregisterSerializer<BASE_NS::Uid>(data);
    UnregisterSerializer<BASE_NS::Math::Mat4X4>(data);
    UnregisterSerializer<BASE_NS::Math::Mat3X3>(data);
    UnregisterSerializer<BASE_NS::Math::Quat>(data);
    UnregisterSerializer<BASE_NS::Math::IVec4>(data);
    UnregisterSerializer<BASE_NS::Math::IVec3>(data);
    UnregisterSerializer<BASE_NS::Math::IVec2>(data);
    UnregisterSerializer<BASE_NS::Math::UVec4>(data);
    UnregisterSerializer<BASE_NS::Math::UVec3>(data);
    UnregisterSerializer<BASE_NS::Math::UVec2>(data);
    UnregisterSerializer<BASE_NS::Math::Vec4>(data);
    UnregisterSerializer<BASE_NS::Math::Vec3>(data);
    UnregisterSerializer<BASE_NS::Math::Vec2>(data);
}

} // namespace Internal
META_END_NAMESPACE()
