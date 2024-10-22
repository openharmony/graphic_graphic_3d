/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "testing_objects.h"

#include <ostream>

#include <base/math/vector.h>

#include <meta/api/call_context.h>
#include <meta/api/util.h>
#include <meta/ext/animation/interpolator.h>
#include <meta/ext/any_builder.h>
#include <meta/ext/attachment/attachment.h>
#include <meta/ext/event_impl.h>
#include <meta/ext/object.h>
#include <meta/ext/object_container.h>
#include <meta/ext/serialization/common_value_serializers.h>
#include <meta/interface/intf_containable.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/intf_startable.h>
#include <meta/interface/intf_tickable.h>
#include <meta/interface/property/intf_property.h>
#include <meta/interface/property/property.h>
#include <meta/interface/property/property_events.h>

namespace {

class TestAttachment
    : public META_NS::AttachmentFwd<TestAttachment, ClassId::TestAttachment, META_NS::ITestAttachment> {
    bool AttachTo(const META_NS::IAttach::Ptr& target, const META_NS::IObject::Ptr& dataContext) override
    {
        attachCount_++;
        return true;
    }
    bool DetachFrom(const META_NS::IAttach::Ptr& target) override
    {
        detachCount_++;
        return true;
    }

    uint32_t GetAttachCount() const override
    {
        return attachCount_;
    }
    uint32_t GetDetachCount() const override
    {
        return detachCount_;
    }

    META_IMPLEMENT_PROPERTY(string, Name);

    BASE_NS::string GetName() const override
    {
        auto name = META_ACCESS_PROPERTY_VALUE(Name);
        return name.empty() ? ObjectFwd::GetName() : name;
    }
    void SetName(const BASE_NS::string& name) override
    {
        META_ACCESS_PROPERTY(Name)->SetValue(name);
    }
private:
    uint32_t attachCount_ {};
    uint32_t detachCount_ {};
};
class TestStartable : public META_NS::AttachmentFwd<TestStartable, ClassId::TestStartable,
                            META_NS::ITestStartable, META_NS::IStartable, META_NS::ITickable> {
    bool AttachTo(const META_NS::IAttach::Ptr& target, const META_NS::IObject::Ptr& dataContext) override
    {
        if (recording_) {
            ops_.push_back(ITestStartable::Operation::ATTACH);
        }
        attachCount_++;
        META_ACCESS_PROPERTY(StartableState)->SetValue(StartableState::ATTACHED);
        return true;
    }
    bool DetachFrom(const META_NS::IAttach::Ptr& target) override
    {
        if (recording_) {
            ops_.push_back(ITestStartable::Operation::DETACH);
        }
        META_ACCESS_PROPERTY(StartableState)->SetValue(StartableState::DETACHED);
        detachCount_++;
        return true;
    }
    uint32_t GetAttachCount() const override
    {
        return attachCount_;
    }
    uint32_t GetDetachCount() const override
    {
        return detachCount_;
    }

    META_IMPLEMENT_PROPERTY(string, Name);
    META_IMPLEMENT_EVENT(IOnChanged, OnStarted)
    META_IMPLEMENT_EVENT(IOnChanged, OnStopped)
    META_IMPLEMENT_EVENT(IOnChanged, OnTicked)

    BASE_NS::string GetName() const override
    {
        auto name = META_ACCESS_PROPERTY_VALUE(Name);
        return name.empty() ? ObjectFwd::GetName() : name;
    }
    void SetName(const BASE_NS::string& name) override
    {
        META_ACCESS_PROPERTY(Name)->SetValue(name);
    }

public:
    void StartRecording() override
    {
        recording_ = true;
        ops_.clear();
    }
    BASE_NS::vector<Operation> StopRecording() override
    {
        recording_ = false;
        return ops_;
    }
    BASE_NS::vector<Operation> GetOps() const override
    {
        return ops_;
    }

public:
    void Tick(const TimeSpan& time, const TimeSpan& sinceLastTick) override
    {
        lastTick_ =  time;
        if (recording_) {
            ops_.push_back(ITestStartable::Operation::TICK);
        }
        META_ACCESS_EVENT(OnTicked)->Invoke();
    }

    TimeSpan GetLastTick() const override
    {
        return lastTick_;
    }

public:
    META_IMPLEMENT_INTERFACE_PROPERTY(IStartable, META_NS::StartBehavior, StartableMode, StartBehavior::MANUAL);
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(
        IStartable, META_NS::StartableState, StartableState, StartableState::DETACHED);

    bool Start() override
    {
        if (recording_) {
            ops_.push_back(ITestStartable::Operation::START);
        }
        META_ACCESS_PROPERTY(StartableState)->SetValue(StartableState::STARTED);
        META_ACCESS_EVENT(OnStarted)->Invoke();
        return true;
    }

    bool Stop() override
    {
        if (recording_) {
            ops_.push_back(ITestStartable::Operation::STOP);
        }
        META_ACCESS_PROPERTY(StartableState)->SetValue(StartableState::ATTACHED);
        META_ACCESS_EVENT(OnStopped)->Invoke();
        return true;
    }

private:
    bool recording_ { false };
    BASE_NS::vector<ITestStartable::Operation> ops_;
    uint32_t attachCount_ {};
    uint32_t detachCount_ {};
    TimeSpan lastTick_ { TimeSpan::Infinite() };
};

class EmbeddedTestType
    : public ObjectFwd<EmbeddedTestType, ClassId::EmbeddedTestType, ClassId::Object, IEmbeddedTestType> {
public:
    META_IMPLEMENT_PROPERTY(int, Property, 0);
};

class TestType : public ObjectFwd<TestType, ClassId::TestType, ClassId::Object, ITestType, IContainable,
                      IMutableContainable, IEnablePropertyTraversal, INotifyOnChange> {
public:
    using Super = ObjectFwd<TestType, ClassId::TestType, ClassId::Object, ITestType, IContainable, IMutableContainable,
        IEnablePropertyTraversal, INotifyOnChange>;

    using Super::Super;

    META_IMPLEMENT_INTERFACE_PROPERTY(ITestType, int, First);

    META_IMPLEMENT_PROPERTY(string, Second, "", ObjectFlagBits::INTERNAL);

    META_IMPLEMENT_READONLY_PROPERTY(int, Third, 0);
    META_IMPLEMENT_READONLY_PROPERTY(string, Fourth, "");

    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec3, Vec3Property1);
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec3, Vec3Property2);
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec3, Vec3Property3);
    META_IMPLEMENT_PROPERTY(float, FloatProperty1);
    META_IMPLEMENT_PROPERTY(float, FloatProperty2, {});
    META_IMPLEMENT_PROPERTY(MyComparableTestType, MyTestTypeProperty1);
    META_IMPLEMENT_PROPERTY(MyComparableTestType, MyTestTypeProperty2);

    META_IMPLEMENT_PROPERTY(IEmbeddedTestType::Ptr, EmbeddedTestTypeProperty);

    META_IMPLEMENT_INTERFACE_ARRAY_PROPERTY(ITestType, int, MyIntArray);
    META_IMPLEMENT_READONLY_ARRAY_PROPERTY(int, MyConstIntArray, (BASE_NS::vector<int> { 1, 2, 3, 4, 5}));

    META_IMPLEMENT_PROPERTY(string, Name);

    META_IMPLEMENT_FUNCTION(TestType, MyTestFunc, "value")
    META_IMPLEMENT_FUNCTION(TestType, MyConstTestFunc, "value")

    META_IMPLEMENT_INTERFACE_EVENT(ITestType, IOnChanged, OnTest);
    META_IMPLEMENT_EVENT(IEvent, IOnChanged, OnChanged);

    int MyTestFunc(int v)
    {
        return v;
    }

    int MyConstTestFunc(int v) const
    {
        return v;
    }

    int NormalMember() override
    {
        return First()->GetValue();
    }

    int OtherNormalMember(int v, int some) const override
    {
        return v + some;
    }

    META_IMPLEMENT_INTERFACE_FUNCTION(ITestType, TestType, NormalMember);
    META_IMPLEMENT_FUNCTION(TestType, OtherNormalMember, "value", "some");

public:
    bool Build(const IMetadata::Ptr&) override
    {
        metaPropertyEmbeddedTestTypeProperty_->SetValue(
            META_NS::GetObjectRegistry().Create<IEmbeddedTestType>(ClassId::EmbeddedTestType));
        return true;
    }

    IObject::Ptr GetParent() const override
    {
        return parent_.lock();
    }

    void SetParent(const IObject::Ptr& parent) override
    {
        parent_ = parent;
    }

    BASE_NS::string GetName() const override
    {
        return Name()->GetValue().empty() ? ObjectFwd::GetName() : Name()->GetValue();
    }

    void SetName(const BASE_NS::string& name) override
    {
        Name()->SetValue(name);
        META_ACCESS_EVENT(OnChanged)->Invoke();
    }

    void SetProperty(const IProperty::ConstWeakPtr& p) override
    {
        owningProp_ = p;
    }

    IProperty::ConstWeakPtr GetProperty() const override
    {
        return owningProp_;
    }

private:
    IObject::WeakPtr parent_;
    IProperty::ConstWeakPtr owningProp_;
};
template<typename FinalClass, const META_NS::ClassInfo& ClassInfo, const META_NS::ClassInfo& SuperClassInfo>
class TestContainerFwd : public CommonObjectContainerFwd<FinalClass, ClassInfo, SuperClassInfo, ITestContainer,
                            IContainable, IMutableContainable> {
    using Super = CommonObjectContainerFwd<FinalClass, ClassInfo, SuperClassInfo, ITestContainer, IContainable,
        IMutableContainable>;

public:
    META_IMPLEMENT_PROPERTY(string, Name);

    IObject::Ptr GetParent() const override
    {
        return parent_.lock();
    }

    void SetParent(const IObject::Ptr& parent) override
    {
        parent_ = parent;
    }

    BASE_NS::string GetName() const override
    {
        return this->Name()->GetValue().empty() ? Super::GetName() : this->Name()->GetValue();
    }

    void SetName(const BASE_NS::string& name) override
    {
        META_ACCESS_PROPERTY(Name)->SetValue(name);
    }

    void Increment() override
    {
        ++increment_;
    }
    int GetIncrement() const override
    {
        return increment_;
    }

    META_IMPLEMENT_FUNCTION(TestContainerFwd, Increment);

private:
    IObject::WeakPtr parent_;
    int increment_ {};
};

class TestContainer : public TestContainerFwd<TestContainer, ClassId::TestContainer, ClassId::ObjectContainer> {};

class TestFlatContainer
    : public TestContainerFwd<TestFlatContainer, ClassId::TestFlatContainer, ClassId::ObjectFlatContainer> {};

class TestString : public MetaObjectFwd<TestString, ClassId::TestString, ClassId::Object, ITestString, IAny, IValue,
                        INotifyOnChange> {
public:
    META_IMPLEMENT_EVENT(IEvent, IOnChanged, OnChanged);

    explicit TestString(BASE_NS::string s = "") : str_(s) {}

    BASE_NS::string GetString() const override
    {
        return str_;
    }
    void SetString(BASE_NS::string s) override
    {
        str_ = s;
        META_ACCESS_EVENT(OnChanged)->Invoke();
    }

    static constexpr TypeId TYPE_ID = UidFromType<ITestString::Ptr>();

    const BASE_NS::array_view<const TypeId> GetCompatibleTypes(CompatibilityDirection) const override
    {
        static const TypeId arr[] = { TYPE_ID, UidFromType<BASE_NS::string>() };
        return arr;
    }

    AnyReturnValue GetData(const TypeId& id, void* data, size_t size) const override
    {
        if (id == TYPE_ID) {
            *static_cast<ITestString::Ptr*>(data) = GetSelf<ITestString>();
            return AnyReturn::SUCCESS;
        }
        if (id == UidFromType<BASE_NS::string>()) {
            *static_cast<BASE_NS::string*>(data) = str_;
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::INCOMPATIBLE_TYPE;
    }
    AnyReturnValue SetData(const TypeId& id, const void* data, size_t size) override
    {
        if (id == TYPE_ID) {
            str_ = (*static_cast<const ITestString::Ptr*>(data))->GetString();
            return AnyReturn::SUCCESS;
        }
        if (id == UidFromType<BASE_NS::string>()) {
            str_ = *static_cast<const BASE_NS::string*>(data);
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::INCOMPATIBLE_TYPE;
    }
    AnyReturnValue CopyFrom(const IAny& any) override
    {
        if (any.GetTypeId() == TYPE_ID) {
            ITestString::Ptr p;
            auto ret = any.GetValue(p);
            if (ret) {
                if (p) {
                    str_ = p->GetString();
                } else {
                    str_ = "";
                }
            }
            return ret;
        }
        return any.GetValue(str_);
    }
    IAny::Ptr Clone(const AnyCloneOptions& options) const override
    {
        if (options.role == TypeIdRole::CURRENT || options.role == TypeIdRole::ITEM) {
            auto obj = META_NS::GetObjectRegistry().Create<ITestString>(ClassId::TestString);
            if (obj && options.value == CloneValueType::COPY_VALUE) {
                obj->SetString(str_);
            }
            return interface_pointer_cast<IAny>(obj);
        }
        return nullptr;
    }
    TypeId GetTypeId(TypeIdRole role) const override
    {
        if (role == TypeIdRole::CURRENT || role == TypeIdRole::ITEM) {
            return TYPE_ID;
        }
        return {};
    }
    BASE_NS::string GetTypeIdString() const override
    {
        return MetaType<ITestString::Ptr>::name;
    }

    AnyReturnValue SetValue(const IAny& value) override
    {
        return CopyFrom(value);
    }
    const IAny& GetValue() const override
    {
        return *this;
    }
    bool IsCompatible(const TypeId& id) const override
    {
        return META_NS::IsCompatible(*this, id);
    }

private:
    BASE_NS::string str_;
};

class TestPtrValue : public ObjectFwd<TestPtrValue, ClassId::TestPtrValue, ClassId::Object, ITestPtrValue> {
public:
    META_IMPLEMENT_PROPERTY(META_VALUE_PTR(ITestString, ClassId::TestString), Text, BASE_NS::string("Some"));
    META_IMPLEMENT_PROPERTY(META_VALUE_PTR(ITestString, ClassId::TestString), String);
};
} // namespace

class MyTestTypeInterpolator
    : public META_NS::Interpolator<MyComparableTestType, MyTestTypeInterpolator, ClassId::MyTestTypeInterpolator> {};

static void RegisterComparableTestTypeSerialiser()
{
    RegisterSerializer<MyComparableTestType>(
        [](auto&, const MyComparableTestType& v) {
            auto n = GetObjectRegistry().Create<IDoubleNode>(META_NS::ClassId::DoubleNode);
            if (n) {
                n->SetValue(v.i);
            }
            return n;
        },
        [](auto&, const ISerNode::ConstPtr& node, MyComparableTestType& out) {
            float v {};
            if (ExtractNumber(node, v)) {
                out.i = v;
                return true;
            }
            return false;
        });
}
static void RegisterTestEnumSerialiser()
{
    RegisterSerializer<TestEnum>(
        [](auto&, const TestEnum& v) {
            auto n = GetObjectRegistry().Create<IUIntNode>(META_NS::ClassId::UIntNode);
            if (n) {
                n->SetValue(v);
            }
            return n;
        },
        [](auto&, const ISerNode::ConstPtr& node, TestEnum& out) {
            auto v = interface_cast<IUIntNode>(node);
            if (v) {
                out = static_cast<TestEnum>(v->GetValue());
            }
            return v != nullptr;
        });
}

static void RegisterMyTestTypeSerialiser()
{
    RegisterSerializer<MyTestType>(
        [](auto&, const MyTestType& v) {
            auto obj = CreateObjectNode(UidFromType<MyTestType>(), "MyTestType");
            if (obj) {
                if (auto n = GetObjectRegistry().Create<IMapNode>(META_NS::ClassId::MapNode)) {
                    auto in = GetObjectRegistry().Create<IIntNode>(META_NS::ClassId::IntNode);
                    if (in) {
                        in->SetValue(v.i);
                        n->AddNode("value", n);
                        obj->SetMembers(n);
                    }
                }
            }
            return obj;
        },
        [](auto&, const ISerNode::ConstPtr& node, MyTestType& out) {
            if (auto obj = interface_cast<IObjectNode>(node)) {
                if (auto m = interface_cast<IMapNode>(obj->GetMembers())) {
                    auto vnode = m->FindNode("value");
                    int64_t v {};
                    if (ExtractNumber(vnode, v)) {
                        out.i = v;
                        return true;
                    }
                }
            }
            return false;
        });
}

void RegisterTestTypes()
{
    auto& registry = META_NS::GetObjectRegistry();
    registry.RegisterObjectType(EmbeddedTestType::GetFactory());
    registry.RegisterObjectType(TestContainer::GetFactory());
    registry.RegisterObjectType(TestFlatContainer::GetFactory());
    registry.RegisterObjectType(TestType::GetFactory());
    registry.RegisterObjectType(MyTestTypeInterpolator::GetFactory());
    registry.RegisterObjectType(TestAttachment::GetFactory());
    registry.RegisterObjectType(TestStartable::GetFactory());
    registry.RegisterObjectType(TestString::GetFactory());
    registry.RegisterObjectType(TestPtrValue::GetFactory());

    RegisterTypeForBuiltinAny<MyComparableTestType>();
    RegisterTypeForBuiltinAny<MyTestType>();
    RegisterTypeForBuiltinAny<TestEnum>();
    RegisterTypeForBuiltinAny<IEmbeddedTestType::Ptr>();
    RegisterTypeForBuiltinAny<ITestType::Ptr>();
    RegisterTypeForBuiltinAny<ITestType::WeakPtr>();

    registry.RegisterInterpolator(UidFromType<MyComparableTestType>(), ClassId::MyTestTypeInterpolator.Id().ToUid());

    RegisterComparableTestTypeSerialiser();
    RegisterTestEnumSerialiser();
    RegisterMyTestTypeSerialiser();
}

void UnregisterTestTypes()
{
    auto& registry = META_NS::GetObjectRegistry();
    UnregisterSerializer<MyTestType>();
    UnregisterSerializer<TestEnum>();
    UnregisterSerializer<MyComparableTestType>();

    registry.UnregisterInterpolator(UidFromType<MyComparableTestType>());

    UnregisterTypeForBuiltinAny<ITestType::WeakPtr>();
    UnregisterTypeForBuiltinAny<ITestType::Ptr>();
    UnregisterTypeForBuiltinAny<IEmbeddedTestType::Ptr>();
    UnregisterTypeForBuiltinAny<MyComparableTestType>();
    UnregisterTypeForBuiltinAny<MyTestType>();
    UnregisterTypeForBuiltinAny<TestEnum>();

    registry.UnregisterObjectType(TestPtrValue::GetFactory());
    registry.UnregisterObjectType(TestString::GetFactory());
    registry.UnregisterObjectType(TestStartable::GetFactory());
    registry.UnregisterObjectType(TestAttachment::GetFactory());
    registry.UnregisterObjectType(MyTestTypeInterpolator::GetFactory());
    registry.UnregisterObjectType(TestType::GetFactory());
    registry.UnregisterObjectType(TestFlatContainer::GetFactory());
    registry.UnregisterObjectType(TestContainer::GetFactory());
    registry.UnregisterObjectType(EmbeddedTestType::GetFactory());
}

ITestType::Ptr CreateTestType(const BASE_NS::string_view name)
{
    const auto object = META_NS::GetObjectRegistry().Create<ITestType>(ClassId::TestType);
    if (object) {
        object->SetName(BASE_NS::string(name));
    }
    return object;
}

ITestContainer::Ptr CreateTestContainer(const BASE_NS::string_view name, ClassInfo type)
{
    const auto object = META_NS::GetObjectRegistry().Create<ITestContainer>(type);
    if (object) {
        object->SetName(BASE_NS::string(name));
    }
    return object;
}

ITestAttachment::Ptr CreateTestAttachment(const BASE_NS::string_view name)
{
    const auto object = META_NS::GetObjectRegistry().Create<ITestAttachment>(ClassId::TestAttachment);
    if (object) {
        object->SetName(BASE_NS::string(name));
    }
    return object;
}

ITestStartable::Ptr CreateTestStartable(const BASE_NS::string_view name, META_NS::StartBehavior behavior)
{
    const auto object = META_NS::GetObjectRegistry().Create<ITestStartable>(ClassId::TestStartable);
    if (object) {
        object->SetName(BASE_NS::string(name));
    }
    SetValue(interface_pointer_cast<IStartable>(object)->StartableMode(), behavior);
    return object;
}

std::ostream& operator<<(std::ostream& os, const META_NS::ITestStartable::Operation& op)
{
    switch (op) {
        case ITestStartable::Operation::START:
            return os << "Operation::START";
        case ITestStartable::Operation::STOP:
            return os << "Operation::STOP";
        case ITestStartable::Operation::ATTACH:
            return os << "Operation::ATTACH";
        case ITestStartable::Operation::DETACH:
            return os << "Operation::DETACH";
        case ITestStartable::Operation::TICK:
            return os << "Operation::TICK";
    }
    return os;
}
