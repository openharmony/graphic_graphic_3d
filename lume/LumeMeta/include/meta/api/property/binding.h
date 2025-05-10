/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.
 * Description: Binding object
 */

#ifndef META_API_PROPERTY_BINDING_H
#define META_API_PROPERTY_BINDING_H

#include <meta/api/function.h>
#include <meta/base/interface_macros.h>
#include <meta/base/interface_traits.h>
#include <meta/interface/property/property.h>

META_BEGIN_NAMESPACE()

/**
 * @brief The Binding class is a helper class for creating inline bindings for META_NS::Object properties.
 */
class Binding {
public:
    META_NO_COPY(Binding)
    META_DEFAULT_MOVE(Binding)

    Binding() noexcept = default;
    virtual ~Binding() = default;

    explicit Binding(const IProperty::ConstPtr& source) noexcept : source_(source) {}
    explicit Binding(const IFunction::ConstPtr& binding) noexcept : binding_(binding) {}

    /**
     * @brief Called by the framework to initialize the binding.
     */
    virtual bool MakeBind(const IProperty::Ptr& target)
    {
        PropertyLock p(target);
        if (target) {
            if (binding_) {
                return p->SetBind(binding_);
            }
            if (const auto source = source_.lock()) {
                return p->SetBind(source);
            }
            // Reset bind
            p->ResetBind();
            return true;
        }
        CORE_LOG_E("Cannot create binding: Invalid bind target");
        return false;
    }

protected:
    IProperty::ConstWeakPtr source_;
    IFunction::ConstPtr binding_;
};

/**
 * @brief The TypedBinding class is a helper for creating typed binding objects. Usually it is enough
 *        to use the Binding class, TypedBinding is mostly helpful for cleaner syntax when defining
 *        bindings with a lambda function.
 */
template<class Type>
class TypedBinding : public Binding {
public:
    using PropertyType = Property<BASE_NS::remove_const_t<Type>>;

    META_NO_COPY_MOVE(TypedBinding)

    ~TypedBinding() override = default;
    TypedBinding() noexcept = default;

    explicit TypedBinding(const Property<const Type>& source) noexcept : Binding(source) {}
    explicit TypedBinding(const PropertyType& source) noexcept : Binding(source) {}

    template<class Callback, typename = EnableIfBindFunction<Callback>>
    explicit TypedBinding(Callback&& callback) : Binding(META_NS::CreateBindFunction(BASE_NS::move(callback)))
    {}
};

META_END_NAMESPACE()

#endif
