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

#ifndef META_EXT_CONRETE_BASE_OBJECT_H
#define META_EXT_CONRETE_BASE_OBJECT_H

#include <base/util/uid_util.h>

#include <meta/base/interface_macros.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/metadata_helpers.h>
#include <meta/ext/object_factory.h>

META_BEGIN_NAMESPACE()

/**
 * @brief The ConcreteBaseMetaObjectFwd class is a helper for creating a class which directly inherits a MetaObject
 *        class defined in the same plugin.
 */
template<class FinalClass, class ConcreteBaseClass, const META_NS::ClassInfo& ClassInfo, class... Interfaces>
class ConcreteBaseMetaObjectFwd : public META_NS::IntroduceInterfaces<Interfaces...>, public ConcreteBaseClass {
protected:
    using IntroducedInterfaces = META_NS::IntroduceInterfaces<Interfaces...>;
    using ConcreteBase = ConcreteBaseClass;

    STATIC_METADATA_WITH_CONCRETE_BASE(IntroducedInterfaces, ConcreteBaseClass)
    STATIC_INTERFACES_WITH_CONCRETE_BASE(IntroducedInterfaces, ConcreteBaseClass)
    META_DEFINE_OBJECT_TYPE_INFO(FinalClass, ClassInfo)

protected: // IObject
    ObjectId GetClassId() const override
    {
        return ClassInfo.Id();
    }
    BASE_NS::string_view GetClassName() const override
    {
        return ClassInfo.Name();
    }
    BASE_NS::vector<BASE_NS::Uid> GetInterfaces() const override
    {
        return GetStaticInterfaces();
    }

public: // CORE_NS::IInterface
    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const override
    {
        auto* me = const_cast<ConcreteBaseMetaObjectFwd*>(this);
        return me->ConcreteBaseMetaObjectFwd::GetInterface(uid);
    }
    CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) override
    {
        CORE_NS::IInterface* ret = ConcreteBase::GetInterface(uid);
        if (!ret) {
            ret = IntroducedInterfaces::GetInterface(uid);
        }
        return ret;
    }

protected:
    void Ref() override
    {
        ConcreteBase::Ref();
    }
    void Unref() override
    {
        ConcreteBase::Unref();
    }
};

META_END_NAMESPACE()

#endif // META_EXT_CONRETE_BASE_OBJECT_H
