#ifndef META_EXT_OBJECT_FACTORY_H
#define META_EXT_OBJECT_FACTORY_H

#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_object_factory.h>
#include <meta/interface/intf_object_registry.h>

#if defined(GetClassInfo)
#undef GetClassInfo
#endif

META_BEGIN_NAMESPACE()

/**
 * @brief A base class which can be used to implement custom object factories.
 */
class ObjectFactory : public IObjectFactory {
    // Ignore refcounts as DefaultObjectFactory is always created as static object
    // for the class for which it implements a factory.
    void Ref() override {}
    void Unref() override {}

public:
    ObjectFactory(const META_NS::ClassInfo& info) : info_(info) {}
    CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) override
    {
        if (uid == CORE_NS::IInterface::UID) {
            return this;
        }
        if (uid == IClassInfo::UID) {
            return this;
        }
        if (uid == IObjectFactory::UID) {
            return this;
        }
        return nullptr;
    }
    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const override
    {
        return const_cast<ObjectFactory*>(this)->GetInterface(uid);
    }
    const META_NS::ClassInfo& GetClassInfo() const override
    {
        return info_;
    }

private:
    META_NS::ClassInfo info_;
};

template<typename FinalClass>
class PlainObjectFactory : public ObjectFactory {
public:
    PlainObjectFactory(const META_NS::ClassInfo& info) : ObjectFactory(info) {}
    IObject::Ptr CreateInstance() const override
    {
        return IObject::Ptr { new FinalClass };
    }
    ClassConstructionType ConstructionType() const override
    {
        return ClassConstructionType::SIMPLE;
    }
};

/**
 * @brief The DefaultObjectFactory class implements the default object factory used
 *        by ObjectFwd and such. hash
 */
template<typename FinalClass>
class DefaultObjectFactory final : public PlainObjectFactory<FinalClass> {
public:
    DefaultObjectFactory(const META_NS::ClassInfo& info) : PlainObjectFactory<FinalClass>(info) {}
    const StaticObjectMetadata* GetClassStaticMetadata() const override
    {
        return FinalClass::StaticMetadata();
    }
    const BASE_NS::vector<BASE_NS::Uid>& GetClassInterfaces() const override
    {
        static auto interfaces = FinalClass::GetInterfacesVector();
        return interfaces;
    }
    ClassConstructionType ConstructionType() const override
    {
        return ClassConstructionType::BUILD;
    }
};

/**
 *  Helper macro for creating ObjectTypeInfo with machinery to register a class using the default object
 *  factory and creating instances of it.
 */
#define META_DEFINE_OBJECT_TYPE_INFO(FinalClass, ClassInfo)                                                      \
    friend class META_NS::DefaultObjectFactory<FinalClass>;                                                      \
                                                                                                                 \
public:                                                                                                          \
    static META_NS::IObjectFactory::Ptr GetFactory()                                                             \
    {                                                                                                            \
        static META_NS::DefaultObjectFactory<FinalClass> factory(ClassInfo);                                     \
        return META_NS::IObjectFactory::Ptr { &factory };                                                        \
    }                                                                                                            \
    static constexpr const META_NS::ObjectTypeInfo OBJECT_INFO { { META_NS::ObjectTypeInfo::UID }, GetFactory }; \
                                                                                                                 \
private:

META_END_NAMESPACE()

#endif
