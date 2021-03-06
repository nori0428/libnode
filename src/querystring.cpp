// Copyright (c) 2012 Plenluno All rights reserved.

#include <assert.h>
#include <libj/js_array.h>
#include <libj/string_buffer.h>

#include "libnode/querystring.h"
#include "libnode/util.h"

namespace libj {
namespace node {
namespace querystring {

static String::CPtr toString(Value val) {
    if (val.isUndefined() ||
        val.equals(Object::null()) ||
        val.instanceof(Type<JsArray>::id()) ||
        val.instanceof(Type<JsObject>::id())) {
        return String::create();
    } else {
        return util::percentEncode(String::valueOf(val));
    }
}

String::CPtr stringify(JsObject::CPtr obj, Char sep, Char eq) {
    if (!obj) return String::create();

    StringBuffer::Ptr res = StringBuffer::create();
    Set::CPtr keys = obj->keySet();
    Iterator::Ptr i = keys->iterator();
    Boolean first = true;
    while (i->hasNext()) {
        if (first) {
            first = false;
        } else {
            res->appendChar(sep);
        }
        Value key = i->next();
        Value val = obj->get(key);
        if (val.instanceof(Type<JsArray>::id())) {
            JsArray::CPtr ary = toCPtr<JsArray>(val);
            for (Size i = 0; i < ary->length(); i++) {
                if (i) {
                    res->appendChar(sep);
                }
                res->append(toString(key));
                res->appendChar(eq);
                res->append(toString(ary->get(i)));
            }
        } else {
            res->append(toString(key));
            res->appendChar(eq);
            res->append(toString(val));
        }
    }
    return res->toString();
}

static void addPair(JsObject::Ptr obj, String::CPtr key, String::CPtr val) {
    if (obj->containsKey(key)) {
        Value v = obj->get(key);
        if (v.instanceof(Type<String>::id())) {
            JsArray::Ptr ary = JsArray::create();
            ary->add(obj->get(key));
            ary->add(val);
            obj->put(key, ary);
        } else {
            JsArray::Ptr ary = toPtr<JsArray>(v);
            assert(ary);
            ary->add(val);
            obj->put(key, ary);
        }
    } else {
        obj->put(key, val);
    }
}

JsObject::Ptr parse(String::CPtr query, Char sep, Char eq) {
    JsObject::Ptr obj = JsObject::create();
    if (!query || query->length() == 0) {
        return obj;
    }

    Size keyStart = 0;
    Size valStart = 0;
    String::CPtr null = String::null();
    String::CPtr key = null;
    String::CPtr val = null;
    Size len = query->length();
    for (Size i = 0; i < len; i++) {
        if (query->charAt(i) == eq) {
            key = query->substring(keyStart, i);
            valStart = i + 1;
        } else if (query->charAt(i) == sep) {
            if (key) {
                val = query->substring(valStart, i);
            } else {
                key = query->substring(keyStart, i);
                val = String::create();
            }
            addPair(obj,
                util::percentDecode(key),
                util::percentDecode(val));
            keyStart = i + 1;
            key = null;
            val = null;
        }
    }
    if (key) {
        val = query->substring(valStart);
    } else {
        key = query->substring(keyStart);
        val = String::create();
    }
    addPair(obj,
        util::percentDecode(key),
        util::percentDecode(val));
    return obj;
}

}  // namespace querystring
}  // namespace node
}  // namespace libj
