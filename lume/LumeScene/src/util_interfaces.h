
#ifndef SCENE_SRC_UTIL_INTERFACES_H
#define SCENE_SRC_UTIL_INTERFACES_H

#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

class IArrayElementIndex : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IArrayElementIndex, "57d7d2e3-0367-45c2-8d45-1a4cc71cdd17")
public:
    virtual void SetIndex(size_t index) = 0;
};

class ArrayElementIndex : public META_NS::IntroduceInterfaces<IArrayElementIndex> {
public:
    void SetIndex(size_t index) override
    {
        index_ = index;
    }

protected:
    size_t index_ = -1;
};

SCENE_END_NAMESPACE()

#endif