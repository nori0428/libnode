// Copyright (c) 2012 Plenluno All rights reserved.

#include "libnode/http.h"

namespace libj {
namespace node {
namespace http {

Server::Ptr createServer(JsFunction::Ptr requestListener) {
    Server::Ptr srv = Server::create();
    if (requestListener) {
        srv->on(Server::EVENT_REQUEST, requestListener);
    }
    return srv;
}

}  // namespace http
}  // namespace node
}  // namespace libj
