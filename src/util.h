#ifndef UTIL_H
#define UTIL_H

#include <v8.h>
#include <node.h>
#include <allheaders.h>

#define THROW(type, msg) \
    v8::ThrowException(v8::Exception::type(v8::String::New(msg)))

v8::Handle<v8::Object> createBox(Box* box);

#endif
