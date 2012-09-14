#include "image.h"
#include <node_buffer.h>
#include <sstream>

using namespace v8;
using namespace node;

#define THROW(type, msg) \
    ThrowException(Exception::type(String::New(msg)))

Persistent<FunctionTemplate> Image::constructor_template;

bool Image::HasInstance(Handle<Value> val)
{
    if (!val->IsObject()) {
        return false;
    }
    Local<Object> obj = val->ToObject();
    return constructor_template->HasInstance(obj);
}

Pix *Image::Pixels(Handle<Object> obj)
{
    return ObjectWrap::Unwrap<Image>(obj)->pix_;
}

void Image::Init(Handle<Object> target)
{
    constructor_template = Persistent<FunctionTemplate>::New(FunctionTemplate::New(New));
    constructor_template->SetClassName(String::NewSymbol("Image"));
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    Local<ObjectTemplate> proto = constructor_template->PrototypeTemplate();
    proto->Set(String::NewSymbol("write"),
               FunctionTemplate::New(Write)->GetFunction());
    target->Set(String::NewSymbol("Image"),
                Persistent<Function>::New(constructor_template->GetFunction()));
}

Handle<Value> Image::New(Pix *pix)
{
    HandleScope scope;
    Local<Object> instance = constructor_template->GetFunction()->NewInstance();
    Image *obj = ObjectWrap::Unwrap<Image>(instance);
    obj->pix_ = pix;
    return scope.Close(instance);
}

Handle<Value> Image::New(const Arguments &args)
{
    HandleScope scope;
    Pix *pix;
    if (args.Length() == 0) {
        pix = 0;
    } else if (args.Length() == 1 && args[0]->IsString()) {
        String::AsciiValue filename(args[0]->ToString());
        if ((pix = pixRead(*filename)) == NULL) {
            std::stringstream msg;
            msg << "error reading file '" << *filename << "'";
            return THROW(Error, msg.str().c_str());
        }
    } else if (args.Length() == 3 && Buffer::HasInstance(args[0])) {
        Local<Object> buffer = args[0]->ToObject();
        int32_t width = args[1]->Int32Value();
        int32_t height = args[2]->Int32Value();
        size_t length = Buffer::Length(buffer);
        if (width* height * sizeof(l_uint32) != length) {
            return THROW(Error, "Buffer has invalid length");
        }
        pix = pixCreateNoInit(width, height, sizeof(l_uint32) << 3);
        memcpy(pix->data, Buffer::Data(buffer), length);
    } else {
        return THROW(TypeError, "could not convert arguments");
    }
    Image* obj = new Image(pix);
    obj->Wrap(args.This());
    return args.This();
}

Handle<Value> Image::Write(const Arguments &args)
{
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 1 && args[0]->IsString()) {
        String::AsciiValue filename(args[0]->ToString());
        if (pixWritePng(*filename, obj->pix_, 1.0f) != 0) {
            std::stringstream msg;
            msg << "error writing file '" << *filename << "'";
            return THROW(Error, msg.str().c_str());
        }
    } else {
        return THROW(TypeError, "could not convert arguments");
    }
    return args.This();
}

Image::Image(Pix *pix)
    : pix_(pix)
{
    V8::AdjustAmountOfExternalAllocatedMemory(size());
}

Image::~Image()
{
    V8::AdjustAmountOfExternalAllocatedMemory(-size());
    if (pix_) {
        pixDestroy(&pix_);
    }
}

int Image::size() const
{
    if (pix_) {
        return pix_->w * pix_->h * (pix_->d >> 3);
    } else {
        return 0;
    }
}
