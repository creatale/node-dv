/*
 * Copyright (c) 2012 Christoph Schulz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "util.h"
#include <cmath>
#include <cstring>

using namespace v8;

Handle<Object> createBox(Box* box)
{
    Handle<Object> result = Object::New();
    result->Set(String::NewSymbol("x"), Int32::New(box->x));
    result->Set(String::NewSymbol("y"), Int32::New(box->y));
    result->Set(String::NewSymbol("width"), Int32::New(box->w));
    result->Set(String::NewSymbol("height"), Int32::New(box->h));
    return result;
}

Box* toBox(const Arguments &args, int start, int* end)
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
        int x = floor(object->Get(String::NewSymbol("x"))->ToNumber()->Value());
        int y = floor(object->Get(String::NewSymbol("y"))->ToNumber()->Value());
        int width = ceil(object->Get(String::NewSymbol("width"))->ToNumber()->Value());
        int height = ceil(object->Get(String::NewSymbol("height"))->ToNumber()->Value());
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
    String::AsciiValue op(value->ToString());
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
