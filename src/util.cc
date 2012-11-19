#include "util.h"

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
