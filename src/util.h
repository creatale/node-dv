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

#include <node.h>
#include <v8.h>
#include <nan.h>
#include <allheaders.h>

v8::Local<v8::Object> createBox(Box* box);
Box* toBox(Nan::NAN_METHOD_ARGS_TYPE args, int start, int* end = 0);
int toOp(v8::Local<v8::Value> value);

#endif
