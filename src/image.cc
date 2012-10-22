#include "image.h"
#include <node_buffer.h>
#include <lodepng.h>
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
    proto->Set(String::NewSymbol("toBuffer"),
               FunctionTemplate::New(ToBuffer)->GetFunction());
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
    } else if (args.Length() == 2 && Buffer::HasInstance(args[1])) {
        String::AsciiValue format(args[0]->ToString());
        Local<Object> buffer = args[1]->ToObject();
        if (strcmp("png", *format) != 0) {
            std::stringstream msg;
            msg << "invalid bufffer format '" << *format << "'";
            return THROW(Error, msg.str().c_str());
        }
        std::vector<unsigned char> out;
        unsigned int width;
        unsigned int height;
        lodepng::State state;
        unsigned char *in = reinterpret_cast<unsigned char*>(Buffer::Data(buffer));
        size_t length = Buffer::Length(buffer);
        unsigned error = lodepng::decode(out, width, height, state, in, length);
        if (error) {
            std::stringstream msg;
            msg << "error while decoding '" << lodepng_error_text(error) << "'";
            return THROW(Error, msg.str().c_str());
        }
        pix = pixCreateNoInit(width, height, 32);
        memcpy(pix->data, &out[0], out.size());
    } else if (args.Length() == 4 && Buffer::HasInstance(args[1])) {
        String::AsciiValue format(args[0]->ToString());
        Local<Object> buffer = args[1]->ToObject();
        int32_t width = args[2]->Int32Value();
        int32_t height = args[3]->Int32Value();
        size_t length = Buffer::Length(buffer);
        if (strcmp("rgba", *format) != 0) {
            std::stringstream msg;
            msg << "invalid bufffer format '" << *format << "'";
            return THROW(Error, msg.str().c_str());
        }
        if (width * height * sizeof(l_uint32) != length) {
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

Handle<Value> Image::ToBuffer(const Arguments &args)
{
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 1 && args[0]->IsString()) {
        String::AsciiValue format(args[0]->ToString());
        if (strcmp("png", *format) != 0) {
            std::stringstream msg;
            msg << "invalid bufffer format '" << *format << "'";
            return THROW(Error, msg.str().c_str());
        }
        std::vector<unsigned char> out;
        unsigned char *inPtr = reinterpret_cast<unsigned char*>(obj->pix_->data);
        std::vector<unsigned char> in(inPtr, inPtr + 4 * obj->pix_->wpl * obj->pix_->h);
        lodepng::State state;
        unsigned error = lodepng::encode(out, in, obj->pix_->w, obj->pix_->h, state);
        if (error) {
            std::stringstream msg;
            msg << "error while encoding '" << lodepng_error_text(error) << "'";
            return THROW(Error, msg.str().c_str());
        }
        return Buffer::New(reinterpret_cast<char *>(&out[0]), out.size())->handle_;
    } else {
        return THROW(TypeError, "could not convert arguments");
    }
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
