/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Description: Helpers for serialisation
 * Author: Mikael Kilpeläinen
 * Create: 2024-01-10
 */

#ifndef META_EXT_SERIALIZATION_SERIALISER_H
#define META_EXT_SERIALIZATION_SERIALISER_H

#include <meta/interface/property/property.h>
#include <meta/interface/serialization/intf_export_context.h>
#include <meta/interface/serialization/intf_import_context.h>

META_BEGIN_NAMESPACE()

template<typename Type>
struct NamedValue {
    NamedValue(BASE_NS::string_view name, Type& v) : name(name), value(v) {}

    BASE_NS::string_view name;
    Type& value;
};

template<typename Type>
NamedValue(BASE_NS::string_view name, const Type& v) -> NamedValue<const Type>;

struct AutoSerializeTag {};
inline AutoSerializeTag AutoSerialize()
{
    return {};
}

class SerializerBase {
public:
    operator ReturnError() const
    {
        return state_;
    }

    explicit operator bool() const
    {
        return state_;
    }

    SerializerBase& SetState(ReturnError s)
    {
        state_ = s;
        return *this;
    }

protected:
    ReturnError state_ { GenericError::SUCCESS };
};

/// Helper class to export and return the status
class ExportSerializer : public SerializerBase {
public:
    ExportSerializer(IExportContext& context) : context_(context) {}

    template<typename Type>
    ExportSerializer& operator&(const NamedValue<Type>& nv)
    {
        if (state_) {
            if constexpr (is_enum_v<BASE_NS::remove_const_t<Type>>) {
                using UT = BASE_NS::underlying_type_t<BASE_NS::remove_const_t<Type>>;
                SetState(context_.ExportValue(nv.name, static_cast<UT>(nv.value)));
            } else {
                SetState(context_.ExportValue(nv.name, nv.value));
            }
        }
        return *this;
    }

    template<typename Type>
    ExportSerializer& operator&(const NamedValue<Property<Type>>& nv)
    {
        if (auto p = interface_pointer_cast<IObject>(nv.value.GetProperty())) {
            *this& NamedValue(nv.name, p);
        } else {
            SetState(GenericError::FAIL);
        }
        return *this;
    }

    template<typename Type>
    ExportSerializer& operator&(const NamedValue<const Property<Type>>& nv)
    {
        if (auto p = interface_pointer_cast<IObject>(nv.value.GetProperty())) {
            *this& NamedValue(nv.name, p);
        } else {
            SetState(GenericError::FAIL);
        }
        return *this;
    }

    template<typename Type>
    ExportSerializer& operator&(const NamedValue<const BASE_NS::weak_ptr<Type>>& nv)
    {
        if (state_) {
            SetState(context_.ExportWeakPtr(nv.name, interface_pointer_cast<IObject>(nv.value)));
        }
        return *this;
    }

    ExportSerializer& operator&(AutoSerializeTag)
    {
        if (state_) {
            SetState(context_.AutoExport());
        }
        return *this;
    }

private:
    IExportContext& context_;
};

/// Helper class to import and return the status
class ImportSerializer : public SerializerBase {
public:
    ImportSerializer(IImportContext& context) : context_(context) {}

    template<typename Type>
    ImportSerializer& operator&(const NamedValue<Type>& nv)
    {
        if (state_) {
            if constexpr (is_enum_v<BASE_NS::remove_const_t<Type>>) {
                using UT = BASE_NS::underlying_type_t<BASE_NS::remove_const_t<Type>>;
                UT v {};
                if (SetState(context_.ImportValue(nv.name, v))) {
                    nv.value = static_cast<Type>(v);
                }
            } else {
                SetState(context_.ImportValue(nv.name, nv.value));
            }
        }
        return *this;
    }

    template<typename Type>
    ImportSerializer& operator&(const NamedValue<Property<Type>>& nv)
    {
        if (auto p = interface_pointer_cast<IObject>(nv.value.GetProperty())) {
            *this& NamedValue(nv.name, p);
        } else {
            SetState(GenericError::FAIL);
        }
        return *this;
    }

    template<typename Type>
    ImportSerializer& operator&(const NamedValue<const Property<Type>>& nv)
    {
        if (auto p = interface_pointer_cast<IObject>(nv.value.GetProperty())) {
            *this& NamedValue(nv.name, p);
        } else {
            SetState(GenericError::FAIL);
        }
        return *this;
    }

    template<typename Type>
    ImportSerializer& operator&(const NamedValue<BASE_NS::weak_ptr<Type>>& nv)
    {
        if (state_) {
            IObject::WeakPtr p;
            SetState(context_.ImportWeakPtr(nv.name, p));
            if (state_) {
                nv.value = interface_pointer_cast<BASE_NS::remove_const_t<Type>>(p);
            }
        }
        return *this;
    }

    ImportSerializer& operator&(AutoSerializeTag)
    {
        if (state_) {
            SetState(context_.AutoImport());
        }
        return *this;
    }

private:
    IImportContext& context_;
};

/// Helper to use same names and syntax for export and import
template<typename Context>
class Serializer {
    Serializer(Context& c);
};

template<>
class Serializer<IImportContext> : public ImportSerializer {
public:
    Serializer(IImportContext& c) : ImportSerializer(c) {}
};

template<>
class Serializer<IExportContext> : public ExportSerializer {
public:
    Serializer(IExportContext& c) : ExportSerializer(c) {}
};

META_END_NAMESPACE()

#endif
