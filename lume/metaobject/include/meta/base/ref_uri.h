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

#ifndef META_BASE_REF_URI_H
#define META_BASE_REF_URI_H

#include <cassert>

#include <base/containers/string.h>
#include <base/containers/vector.h>

#include <meta/base/ids.h>
#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

/**
 * @brief The RefUri class represents a URI that is used to reference objects and properties in
 *        an object hierarchy.
 *
 *        Examples of valid uris:
 *          - "ref:/$Name": Current object's "Name" property
 *          - "ref:/../MyList/$ItemCount": Parent object's child with name "MyList", and the property "ItemCount"
 *            from that object
 *          - "ref:/MyList/$CurrentItem/$ItemName": MyList object's CurrentItem property (which contains an object),
 *            whose ItemName property is accessed
 *          - "ref:1234-5678-9abc-deff/$Name": absolute path to object, object's "Name" property
 *          - "ref:1234-5678-9abc-deff//$Name": absolute path to object, object's hierarchy's root object's "Name"
 *            property (note extra / for "root")
 *          - "ref://$Name": Current object's hierarchy's root object's "Name" property
 *          - "ref:/../$Name": Current object's parent's "Name" property
 *          - "ref:/../AnotherChild/$Name": Current object's sibling AnotherChild's "Name" property
 *          - "ref:/./$Name": Current object's "Name" property
 *          - "ref:/@Context/$Theme": Current object's context's property named Theme
 *          - "ref:/@Theme": Current widget's theme
 *          - "ref:1234-5678-9abc-deff/@Context/$Name": absolute path to object, object's context's "Name" property
 *          - "ref://": Current object's hierarchy's root object
 *          - "ref:/": Current object
 */
class RefUri {
public:
    /** Character identifying properties */
    constexpr static char PROPERTY_CHAR = '$';
    /** Separator character between names */
    constexpr static char SEPARATOR_CHAR = '/';
    /** Escape character */
    constexpr static char ESCAPE_CHAR = '\\';
    /** Characters that are automatically escaped */
    constexpr static BASE_NS::string_view ESCAPED_CHARS = "/@$\\";

    struct Node {
        BASE_NS::string name;
        enum Type { OBJECT, PROPERTY, SPECIAL } type {};
    };

    RefUri();
    /**
     * @brief Construct RefUri by parsing uri.
     */
    explicit RefUri(BASE_NS::string_view uri);
    /**
     * @brief Construct RefUri using baseObject as starting point and parse path.
     */
    RefUri(const InstanceId& baseObject, BASE_NS::string_view path = "/");

    /**
     * @brief Check if this is valid RefUri. This means the associated uri is well-formed,
     * it might or might not point to existing object or property.
     */
    bool IsValid() const;
    /**
     * @brief Check if this is empty RefUri (i.e. default constructed).
     */
    bool IsEmpty() const;
    /**
     * @brief Convert RefUri to string presentation of the associated uri.
     */
    BASE_NS::string ToString() const;
    /**
     * @brief Check if the last segment in the associated uri is property.
     */
    bool ReferencesProperty() const;
    /**
     * @brief Check if the last segment in the associated uri is object.
     */
    bool ReferencesObject() const;
    /**
     * @brief UID of the base object if present.
     */
    InstanceId BaseObjectUid() const;
    /**
     * @brief Sets the base object UID, can be used to replace the existing UID to re-root the path.
     */
    void SetBaseObjectUid(const InstanceId& id);
    /**
     * @brief Check if the associated uri starts from the root object (i.e. it has // in the beginning).
     */
    bool StartsFromRoot() const;
    /**
     * @brief Enable/Disable starting from root object. This can be used to make the RefUri relative to the root.
     */
    void SetStartsFromRoot(bool value);
    /**
     * @brief Returns relative uri to the base object (i.e. effectively drops off the base object).
     */
    RefUri RelativeUri() const;
    /**
     * @brief Removes and returns the first segment in the path.
     */
    Node TakeFirstNode();
    /**
     * @brief Removes and returns the last segment in the path.
     */
    Node TakeLastNode();
    /**
     * @brief Name of the last segment in the uri.
     * Returns empty string for "ref:/.." and "ref:/[/]".
     */
    BASE_NS::string ReferencedName() const;

    /**
     * @brief Add object name as first segment (the first pushed segment will be the last one in the end).
     */
    void PushObjectSegment(BASE_NS::string name);
    /**
     * @brief Add property name as first segment (the first pushed segment will be the last one in the end).
     */
    void PushPropertySegment(BASE_NS::string name);
    /**
     * @brief Add special object context as first segment (the first pushed segment will be the last one in the end).
     */
    void PushObjectContextSegment();

    bool operator==(const RefUri& uri) const;
    bool operator!=(const RefUri& uri) const;

    /** Shorthand for RefUri("ref:/..") */
    static const RefUri& ParentUri();
    /** Shorthand for RefUri("ref:/") */
    static const RefUri& SelfUri();
    /** Shorthand for RefUri("ref:/@Context") */
    static const RefUri& ContextUri();

    /** Return true if ref:/base/$test refers to the property, otherwise (default) it refers to the value of the
     * property */
    bool GetAbsoluteInterpretation() const
    {
        return interpretAbsolute_;
    }
    void SetAbsoluteInterpretation(bool v)
    {
        interpretAbsolute_ = v;
    }

private:
    bool Parse(BASE_NS::string_view uri);
    bool ParsePath(BASE_NS::string_view path);
    bool ParseSegment(BASE_NS::string_view& path);
    bool ParseUid(BASE_NS::string_view& path);
    bool AddSegment(BASE_NS::string seg);

    static BASE_NS::string EscapeName(BASE_NS::string_view str);
    static BASE_NS::string UnEscapeName(BASE_NS::string_view str);

private:
    bool isValid_ {};
    InstanceId baseUid_;
    bool startFromRoot_ {};
    BASE_NS::vector<Node> segments_;
    // this is context specific, not in the string format
    bool interpretAbsolute_ {};
};

inline RefUri::RefUri() : isValid_ { true } {}

inline RefUri::RefUri(BASE_NS::string_view uri)
{
    isValid_ = Parse(uri);
}

inline RefUri::RefUri(const InstanceId& baseObject, BASE_NS::string_view path) : baseUid_(baseObject)
{
    isValid_ = ParsePath(path);
}

inline bool RefUri::IsValid() const
{
    return isValid_;
}

inline bool RefUri::IsEmpty() const
{
    RefUri empty {};
    // ignore the context specific flag for empty check
    empty.SetAbsoluteInterpretation(GetAbsoluteInterpretation());
    return *this == empty;
}

inline bool RefUri::ReferencesProperty() const
{
    return IsValid() && !segments_.empty() && segments_[0].type == Node::PROPERTY;
}

inline bool RefUri::ReferencesObject() const
{
    return IsValid() && !ReferencesProperty();
}

inline RefUri RefUri::RelativeUri() const
{
    RefUri copy { *this };
    copy.SetBaseObjectUid({});
    return copy;
}

inline RefUri::Node RefUri::TakeFirstNode()
{
    Node n = segments_.back();
    segments_.pop_back();
    return n;
}

inline RefUri::Node RefUri::TakeLastNode()
{
    Node n = segments_.front();
    segments_.erase(segments_.begin());
    return n;
}

inline BASE_NS::string RefUri::ReferencedName() const
{
    return segments_.empty() ? "" : segments_.front().name;
}

inline bool RefUri::StartsFromRoot() const
{
    return startFromRoot_;
}

inline void RefUri::SetStartsFromRoot(bool value)
{
    startFromRoot_ = value;
}

inline InstanceId RefUri::BaseObjectUid() const
{
    return baseUid_;
}

inline void RefUri::SetBaseObjectUid(const InstanceId& uid)
{
    baseUid_ = uid;
}

inline void RefUri::PushObjectSegment(BASE_NS::string name)
{
    segments_.push_back(Node { BASE_NS::move(name), Node::OBJECT });
}

inline void RefUri::PushPropertySegment(BASE_NS::string name)
{
    segments_.push_back(Node { BASE_NS::move(name), Node::PROPERTY });
}

inline void RefUri::PushObjectContextSegment()
{
    segments_.push_back(Node { "@Context", Node::SPECIAL });
}

inline BASE_NS::string RefUri::ToString() const
{
    BASE_NS::string res = "ref:";
    if (baseUid_.IsValid()) {
        res += baseUid_.ToString();
    }
    if (startFromRoot_) {
        res += SEPARATOR_CHAR;
    }
    if (segments_.empty()) {
        res += SEPARATOR_CHAR;
    }
    for (auto it = segments_.rbegin(); it != segments_.rend(); ++it) {
        res += SEPARATOR_CHAR;
        if (it->type == Node::PROPERTY) {
            res += PROPERTY_CHAR;
        }
        // don't escape special names
        res += it->type != Node::SPECIAL ? EscapeName(it->name) : it->name;
    }
    return res;
}

inline bool RefUri::operator==(const RefUri& uri) const
{
    // always unequal for non-valid uri
    if (!isValid_ || !uri.isValid_) {
        return false;
    }
    if (baseUid_ != uri.baseUid_) {
        return false;
    }
    if (startFromRoot_ != uri.startFromRoot_) {
        return false;
    }
    if (interpretAbsolute_ != uri.interpretAbsolute_) {
        return false;
    }
    if (segments_.size() != uri.segments_.size()) {
        return false;
    }
    for (auto it1 = segments_.begin(), it2 = uri.segments_.begin(); it1 != segments_.end(); ++it1, ++it2) {
        if (it1->name != it2->name || it1->type != it2->type) {
            return false;
        }
    }
    return true;
}

inline bool RefUri::operator!=(const RefUri& uri) const
{
    return !(*this == uri);
}

inline const RefUri& RefUri::ParentUri()
{
    static const RefUri uri { "ref:/.." };
    return uri;
}

inline const RefUri& RefUri::SelfUri()
{
    static const RefUri uri { "ref:/" };
    return uri;
}

inline const RefUri& RefUri::ContextUri()
{
    static const RefUri uri { "ref:/@Context" };
    return uri;
}

inline bool RefUri::AddSegment(BASE_NS::string seg)
{
    if (seg == ".") {
        // ignore .
    } else if (seg == ".." && !segments_.empty()) {
        // remove last segment if we have one
        segments_.pop_back();
    } else if (seg[0] == PROPERTY_CHAR) {
        seg.erase(0, 1);
        if (seg.empty()) {
            return false;
        }
        PushPropertySegment(UnEscapeName(seg));
    } else if (seg.substr(0, 8) == "@Context") { // 8 count index
        PushObjectContextSegment();
    } else if (seg.substr(0, 8) == "@Theme") { // 8 count index
        PushObjectContextSegment();
        PushPropertySegment("Theme");
    } else {
        PushObjectSegment(UnEscapeName(seg));
    }
    return true;
}

inline bool RefUri::ParseSegment(BASE_NS::string_view& path)
{
    size_t i = 0;
    for (; i < path.size() && path[i] != SEPARATOR_CHAR; ++i) {
        if (path[i] == ESCAPE_CHAR) {
            ++i;
        }
    }
    if (i == 0 || !AddSegment(BASE_NS::string(path.substr(0, i)))) {
        return false;
    }

    path.remove_prefix(i + 1);
    return true;
}

inline bool RefUri::ParsePath(BASE_NS::string_view path)
{
    if (path.empty() || path[0] != SEPARATOR_CHAR) {
        return false;
    }
    path.remove_prefix(1);
    if (path.empty()) {
        return true;
    }
    // see if we have double separator meaning root object
    if (path[0] == SEPARATOR_CHAR) {
        startFromRoot_ = true;
        path.remove_prefix(1);
    }
    while (!path.empty()) {
        if (!ParseSegment(path)) {
            return false;
        }
    }
    // all good, reverse segments and we are done
    BASE_NS::vector<Node> rev { segments_.rbegin(), segments_.rend() };
    segments_ = BASE_NS::move(rev);
    return true;
}

inline bool RefUri::ParseUid(BASE_NS::string_view& path)
{
    static constexpr size_t uidSize = 36;
    if (path.size() < uidSize) {
        return false;
    }
    baseUid_ = BASE_NS::StringToUid(path.substr(0, uidSize));
    path.remove_prefix(uidSize);
    return true;
}

inline bool RefUri::Parse(BASE_NS::string_view uri)
{
    // we check the header and size must be at least 5 (at least / after the header)
    if (uri.size() < 5 || uri.substr(0, 4) != "ref:") { // 5 4 size
        return false;
    }
    uri.remove_prefix(4); // 4 size
    // see if it is path or valid uid
    if (uri[0] != SEPARATOR_CHAR && !ParseUid(uri)) {
        return false;
    }
    if (!uri.empty() && uri[0] == SEPARATOR_CHAR) {
        return ParsePath(uri);
    }
    return uri.empty();
}

inline BASE_NS::string RefUri::EscapeName(BASE_NS::string_view str)
{
    BASE_NS::string res { str };
    for (size_t i = 0; i != res.size(); ++i) {
        if (ESCAPED_CHARS.find(res[i]) != BASE_NS::string_view::npos) {
            res.insert(i, &ESCAPE_CHAR, 1);
            ++i;
        }
    }
    return res;
}

inline BASE_NS::string RefUri::UnEscapeName(BASE_NS::string_view str)
{
    BASE_NS::string res { str };
    for (size_t i = 0; i < res.size(); ++i) {
        if (res[i] == ESCAPE_CHAR) {
            res.erase(i, 1);
        }
    }
    return res;
}

META_TYPE(RefUri);

META_END_NAMESPACE()

#endif
