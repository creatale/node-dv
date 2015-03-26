/*
 * node-dv - Document Vision for node.js
 *
 * Copyright (c) 2012 Christoph Schulz
 * Copyright (c) 2013-2015 creatale GmbH, contributors listed under AUTHORS
 * 
 * MIT License <https://github.com/creatale/node-dv/blob/master/LICENSE>
 */
#ifndef UTIL_H
#define UTIL_H

#include <v8.h>
#include <node.h>
#include <allheaders.h>

#define THROW(type, msg) \
    v8::ThrowException(v8::Exception::type(v8::String::New(msg)))

v8::Handle<v8::Object> createBox(Box* box);
Box* toBox(const v8::Arguments &args, int start, int* end = 0);
int toOp(v8::Local<v8::Value> value);

#endif
