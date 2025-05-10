#ifndef META_SRC_NUMBER_H
#define META_SRC_NUMBER_H

#include <variant>

#include <meta/api/number.h>
#include <meta/interface/intf_any.h>

#include "base_object.h"

META_BEGIN_NAMESPACE()
namespace Internal {

class Number : public IntroduceInterfaces<BaseObject, IAny> {
    META_OBJECT(Number, META_NS::ClassId::Number, IntroduceInterfaces)
public:
    using VariantType = std::variant<int64_t, uint64_t, float>;

    explicit Number(VariantType v = {});

    const BASE_NS::array_view<const TypeId> GetCompatibleTypes(CompatibilityDirection) const override;
    AnyReturnValue GetData(const TypeId& uid, void* data, size_t size) const override;
    AnyReturnValue SetData(const TypeId& uid, const void* data, size_t size) override;
    AnyReturnValue CopyFrom(const IAny& any) override;
    AnyReturnValue ResetValue() override;
    IAny::Ptr Clone(const AnyCloneOptions& options) const override;
    TypeId GetTypeId(TypeIdRole role) const override;
    BASE_NS::string GetTypeIdString() const override;

private:
    VariantType value_;
};

} // namespace Internal
META_END_NAMESPACE()

#endif
