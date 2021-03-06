// Copyright (c) 2012 Plenluno All rights reserved.

#ifndef LIBNODE_SRC_NET_SERVER_IMPL_H_
#define LIBNODE_SRC_NET_SERVER_IMPL_H_

#include "libnode/net/server.h"

#include "./socket_impl.h"
#include "../flag.h"

namespace libj {
namespace node {
namespace net {

class ServerImpl
    : public FlagMixin
    , LIBNODE_NET_SERVER(ServerImpl)
 public:
    static Ptr create() {
        return Ptr(new ServerImpl());
    }

    Value address() {
        if (handle_ && handle_->type() == UV_TCP) {
            uv::Tcp* tcp = static_cast<uv::Tcp*>(handle_);
            return tcp->getSockName();
        } else if (pipeName_) {
            return pipeName_;
        } else {
            return JsObject::null();
        }
    }

    Boolean listen(
        Int port,
        String::CPtr host = IN_ADDR_ANY,
        Int backlog = 511,
        JsFunction::Ptr callback = JsFunction::null()) {
        if (callback) once(EVENT_LISTENING, callback);
        return listen(host, port, 4, backlog);
    }

    Boolean close(
        JsFunction::Ptr callback = JsFunction::null()) {
        if (!handle_) return false;

        if (callback) once(EVENT_CLOSE, callback);

        handle_->close();
        handle_ = NULL;
        emitCloseIfDrained();
        return true;
    }

    void ref() {
        if (handle_) handle_->ref();
    }

    void unref() {
        if (handle_) handle_->unref();
    }

 private:
    static uv::Stream* createServerHandle(
        String::CPtr address,
        Int port = -1,
        Int addressType = -1,
        int fd = -1) {
        uv::Stream* handle;
        if (fd >= 0) {
            uv::Pipe* pipe = new uv::Pipe();
            pipe->open(fd);
            return pipe;
        } else if (port == -1 && addressType == -1) {
            handle = new uv::Pipe();
        } else {
            handle = new uv::Tcp();
        }

        Int r = 0;
        if (address || port) {
            uv::Tcp* tcp = static_cast<uv::Tcp*>(handle);
            if (addressType == 6) {
                r = tcp->bind6(address, port);
            } else {
                r = tcp->bind(address, port);
            }
        }

        if (r) {
            handle->close();
            return NULL;
        } else {
            return handle;
        }
    }

    Boolean listen(
        String::CPtr address,
        Int port,
        Int addressType,
        Int backlog = 0,
        int fd = -1) {
        if (!handle_) {
            handle_ = createServerHandle(address, port, addressType, fd);
            if (!handle_) {
                EmitError::Ptr emitError(new EmitError(this));
                process::nextTick(emitError);
                return false;
            }
        }

        OnConnection::Ptr onConnection(new OnConnection(this));
        handle_->setOnConnection(onConnection);

        Int r = handle_->listen(backlog ? backlog : 511);
        if (r) {
            handle_->close();
            handle_ = NULL;
            EmitError::Ptr emitError(new EmitError(this));
            process::nextTick(emitError);
            return false;
        } else {
            // connectionKey = addressType + ':' + address + ':' + port;
            EmitListening::Ptr emitListening(new EmitListening(this));
            process::nextTick(emitListening);
            return true;
        }
    }

    void emitCloseIfDrained() {
        if (handle_ || connections_) return;

        EmitClose::Ptr emitClose(new EmitClose(this));
        process::nextTick(emitClose);
    }

 private:
    class OnConnection : LIBJ_JS_FUNCTION(OnConnection)
     private:
        ServerImpl* self_;

     public:
        OnConnection(ServerImpl* srv) : self_(srv) {}

        Value operator()(JsArray::Ptr args) {
            Value client = args->get(0);
            uv::Stream* clientHandle = NULL;
            uv::Pipe* pipe;
            uv::Tcp* tcp;
            if (to<uv::Pipe*>(client, &pipe)) {
                clientHandle = pipe;
            } else if (to<uv::Tcp*>(client, &tcp)) {
                clientHandle = tcp;
            }
            if (!clientHandle) {
                EmitError::Ptr emitError(new EmitError(self_));
                process::nextTick(emitError);
                return Error::ILLEGAL_STATE;
            }

            #if 0
            if (self_->maxConnections &&
                self_->connections >= self_->maxConnections) {
                clientHandle->close();
                return;
            }
            #endif

            SocketImpl::Ptr socket = SocketImpl::create(
                clientHandle,
                self_->hasFlag(ALLOW_HALF_OPEN));
            socket->setFlag(SocketImpl::READABLE);
            socket->setFlag(SocketImpl::WRITABLE);

            clientHandle->readStart();

            self_->connections_++;
            // socket.server = self;

            self_->emit(EVENT_CONNECTION, socket);
            socket->emit(SocketImpl::EVENT_CONNECT);
            return Status::OK;
        }
    };

    class EmitClose : LIBJ_JS_FUNCTION(EmitClose)
     private:
        ServerImpl* self_;

     public:
        EmitClose(ServerImpl* srv) : self_(srv) {}

        Value operator()(JsArray::Ptr args) {
            self_->emit(EVENT_CLOSE);
            return Status::OK;
        }
    };

    class EmitError : LIBJ_JS_FUNCTION(EmitError)
     private:
        ServerImpl* self_;

     public:
        EmitError(ServerImpl* srv) : self_(srv) {}

        Value operator()(JsArray::Ptr args) {
            self_->emit(EVENT_ERROR, uv::Error::last());
            return Status::OK;
        }
    };

    class EmitListening : LIBJ_JS_FUNCTION(EmitListening)
     private:
        ServerImpl* self_;

     public:
        EmitListening(ServerImpl* srv) : self_(srv) {}

        Value operator()(JsArray::Ptr args) {
            self_->emit(EVENT_LISTENING);
            return Status::OK;
        }
    };

 public:
    enum Flag {
        ALLOW_HALF_OPEN = 1 << 0,
    };

 private:
    uv::Stream* handle_;
    Size connections_;
    String::CPtr pipeName_;
    events::EventEmitter::Ptr ee_;

    ServerImpl()
        : handle_(NULL)
        , connections_(0)
        , pipeName_(String::null())
        , ee_(events::EventEmitter::create()) {}

    LIBNODE_EVENT_EMITTER_IMPL(ee_);
};

}  // namespace net
}  // namespace node
}  // namespace libj

#endif  // LIBNODE_SRC_NET_SERVER_IMPL_H_
