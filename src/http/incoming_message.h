// Copyright (c) 2012 Plenluno All rights reserved.

#ifndef LIBNODE_SRC_HTTP_INCOMING_MESSAGE_H_
#define LIBNODE_SRC_HTTP_INCOMING_MESSAGE_H_

#include <assert.h>
#include <libj/linked_list.h>

#include "libnode/http/header.h"
#include "libnode/process.h"
#include "libnode/stream/readable_stream.h"
#include "libnode/string_decoder.h"

#include "../flag.h"
#include "../net/socket_impl.h"

namespace libj {
namespace node {
namespace http {

class IncomingMessage
    : public FlagMixin
    , LIBNODE_READABLE_STREAM(IncomingMessage)
 public:
    static Ptr create(net::SocketImpl::Ptr sock) {
        if (sock) {
            return Ptr(new IncomingMessage(sock));
        } else {
            return null();
        }
    }

    net::Socket::Ptr socket() const {
        return socket_;
    }

    net::Socket::Ptr connection() const {
        return socket_;
    }

    Int statusCode() const {
        return statusCode_;
    }

    String::CPtr httpVersion() const {
        return httpVersion_;
    }

    JsObject::CPtr headers() const {
        return headers_;
    }

    String::CPtr url() const {
        return url_;
    }

    String::CPtr method() const {
        return method_;
    }

    Boolean readable() const {
        return hasFlag(READABLE);
    }
    Boolean destroy() {
        return socket_->destroy();
    }

    Boolean setEncoding(Buffer::Encoding enc) {
        decoder_ = StringDecoder::create(enc);
        return !!decoder_;
    }

 public:
    void setStatusCode(Int statusCode) {
        statusCode_ = statusCode;
    }

    void setHttpVersion(String::CPtr httpVersion) {
        httpVersion_ = httpVersion;
    }

    String::CPtr getHeader(String::CPtr name) const {
        return headers_->getCPtr<String>(name);
    }

    void setHeader(String::CPtr name, String::CPtr value) {
        headers_->put(name->toLowerCase(), value);
    }

    void addHeaderLine(String::CPtr name, String::CPtr value) {
        LIBJ_STATIC_SYMBOL_DEF(symExtPrefix, "x-");
        static Set::Ptr commaSeparated = Set::null();

        assert(name && value);

        if (!commaSeparated) {
            commaSeparated = Set::create();
            commaSeparated->add(LHEADER_ACCEPT);
            commaSeparated->add(LHEADER_ACCEPT_CHARSET);
            commaSeparated->add(LHEADER_ACCEPT_ENCODING);
            commaSeparated->add(LHEADER_ACCEPT_LANGUAGE);
            commaSeparated->add(LHEADER_CONNECTION);
            commaSeparated->add(LHEADER_COOKIE);
            commaSeparated->add(LHEADER_PRAGMA);
            commaSeparated->add(LHEADER_LINK);
            commaSeparated->add(LHEADER_WWW_AUTHENTICATE);
            commaSeparated->add(LHEADER_PROXY_AUTHENTICATE);
            commaSeparated->add(LHEADER_SEC_WEBSOCKET_EXTENSIONS);
            commaSeparated->add(LHEADER_SEC_WEBSOCKET_PROTOCOL);
        }

        JsObject::Ptr dest = hasFlag(COMPLETE) ? trailers_ : headers_;
        String::CPtr field = name->toLowerCase();
        if (field->equals(LHEADER_SET_COOKIE)) {
            JsArray::Ptr vals = headers_->getPtr<JsArray>(field);
            if (vals) {
                vals->add(value);
            } else {
                JsArray::Ptr vals = JsArray::create();
                vals->add(value);
                dest->put(field, vals);
            }
        } else if (commaSeparated->contains(field) ||
                   field->startsWith(symExtPrefix)) {
            String::CPtr vals = headers_->getCPtr<String>(field);
            if (vals) {
                StringBuffer::Ptr sb = StringBuffer::create();
                sb->append(vals);
                sb->appendCStr(", ");
                sb->append(value);
                dest->put(field, sb->toString());
            } else {
                dest->put(field, value);
            }
        } else {
            dest->put(field, value);
        }
    }

    void setUrl(String::CPtr url) {
        url_ = url;
    }

    void setMethod(String::CPtr method) {
        method_ = method;
    }

    Boolean hasEncoding() const {
        return !!decoder_;
    }

    Buffer::Encoding getEncoding() const {
        return decoder_ ? decoder_->encoding() : Buffer::NONE;
    }

    LinkedList::Ptr getPendings() const {
        return pendings_;
    }

    void emitPending(JsFunction::Ptr callback = JsFunction::null()) {
        if (pendings_->isEmpty()) {
            if (callback) {
                (*callback)();
            }
        } else {
            process::nextTick(EmitPending::create(this, callback));
        }
    }

    void emitData(Buffer::CPtr buf) {
        if (decoder_) {
            String::CPtr str = decoder_->write(buf);
            if (!str->isEmpty()) {
                emit(EVENT_DATA, str);
            }
        } else {
            emit(EVENT_DATA, buf);
        }
    }

    void emitEnd() {
        if (!hasFlag(END_EMITTED)) {
            emit(EVENT_END);
        }
        setFlag(END_EMITTED);
    }

 public:
    typedef enum {
        COMPLETE    = 1 << 0,
        READABLE    = 1 << 1,
        PAUSED      = 1 << 2,
        END_EMITTED = 1 << 3,
        UPGRADE     = 1 << 4,
    } Flag;

 private:
    class EmitPending : LIBJ_JS_FUNCTION(EmitPending)
     public:
        static Ptr create(
                IncomingMessage* incoming,
                JsFunction::Ptr callback) {
            return Ptr(new EmitPending(incoming, callback));
        }

        Value operator()(JsArray::Ptr args) {
            LinkedList::Ptr pendings = self_->getPendings();
            assert(pendings);
            while (!self_->hasFlag(PAUSED) && self_->getPendings()->length()) {
                Buffer::Ptr chunk = toPtr<Buffer>(pendings->remove(0));
                if (chunk) {
                    assert(Buffer::isBuffer(chunk));
                    self_->emitData(chunk);
                } else {
                    assert(pendings->isEmpty());
                    self_->unsetFlag(READABLE);
                    self_->emitEnd();
                }
            }
            if (callback_)
                (*callback_)();
            return libj::Status::OK;
        }

     private:
        IncomingMessage* self_;
        JsFunction::Ptr callback_;

        EmitPending(
            IncomingMessage* incoming,
            JsFunction::Ptr callback)
            : self_(incoming)
            , callback_(callback) {}
    };

 private:
    net::SocketImpl::Ptr socket_;
    Int statusCode_;
    String::CPtr httpVersion_;
    JsObject::Ptr headers_;
    JsObject::Ptr trailers_;
    String::CPtr url_;
    String::CPtr method_;
    LinkedList::Ptr pendings_;
    StringDecoder::Ptr decoder_;
    EventEmitter::Ptr ee_;

    IncomingMessage(net::SocketImpl::Ptr sock)
        : socket_(sock)
        , statusCode_(0)
        , httpVersion_(String::null())
        , headers_(JsObject::create())
        , trailers_(JsObject::create())
        , url_(String::create())
        , method_(String::null())
        , pendings_(LinkedList::create())
        , decoder_(StringDecoder::null())
        , ee_(EventEmitter::create()) {
        setFlag(READABLE);
    }

    LIBNODE_EVENT_EMITTER_IMPL(ee_);
};

}  // namespace http
}  // namespace node
}  // namespace libj

#endif  // LIBNODE_SRC_HTTP_INCOMING_MESSAGE_H_
