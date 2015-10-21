/*
 * node-dv - Document Vision for node.js
 *
 * Copyright (c) 2012 Christoph Schulz
 * Copyright (c) 2013-2015 creatale GmbH, contributors listed under AUTHORS
 * 
 * MIT License <https://github.com/creatale/node-dv/blob/master/LICENSE>
 */
#include "util.h"
#include <cmath>
#include <cstring>

using namespace v8;

Local<Object> createBox(Box* box)
{
	Nan::EscapableHandleScope scope;
    Local<Object> result = Nan::New<Object>();
    result->Set(Nan::New("x").ToLocalChecked(), Nan::New(box->x));
    result->Set(Nan::New("y").ToLocalChecked(), Nan::New(box->y));
    result->Set(Nan::New("width").ToLocalChecked(), Nan::New(box->w));
    result->Set(Nan::New("height").ToLocalChecked(), Nan::New(box->h));
    return scope.Escape(result);
}

Box* toBox(Nan::NAN_METHOD_ARGS_TYPE args, int start, int* end)
{
    if (args[start]->IsNumber() && args[start + 1]->IsNumber()
            && args[start + 2]->IsNumber() && args[start + 3]->IsNumber()) {
        int x = floor(args[start + 0]->ToNumber()->Value());
        int y = floor(args[start + 1]->ToNumber()->Value());
        int width = ceil(args[start + 2]->ToNumber()->Value());
        int height = ceil(args[start + 3]->ToNumber()->Value());
        if (end) {
            *end = start + 3;
        }
        return boxCreate(x, y, width, height);
    } else if (args[start]->IsObject()) {
        Handle<Object> object = args[start]->ToObject();
        int x = floor(object->Get(Nan::New("x").ToLocalChecked())->ToNumber()->Value());
        int y = floor(object->Get(Nan::New("y").ToLocalChecked())->ToNumber()->Value());
        int width = ceil(object->Get(Nan::New("width").ToLocalChecked())->ToNumber()->Value());
        int height = ceil(object->Get(Nan::New("height").ToLocalChecked())->ToNumber()->Value());
        if (end) {
            *end = start;
        }
        return boxCreate(x, y, width, height);
    } else {
        if (end) {
            *end = start - 1;
        }
        return 0;
    }
}

int toOp(v8::Local<v8::Value> value)
{
    int result  = L_FLIP_PIXELS;
    String::Utf8Value op(value->ToString());
    if (strcmp("set", *op) == 0) {
        result = L_SET_PIXELS;
    } else if (strcmp("clear", *op) == 0) {
        result = L_CLEAR_PIXELS;
    } else if (strcmp("flip", *op) == 0) {
        result = L_FLIP_PIXELS;
    } else {
        result = -1;
    }
    return result;
}
