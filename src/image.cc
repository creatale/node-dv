#include "image.h"
#include <node_buffer.h>
#include <lodepng.h>
#include <sstream>
#include <iostream> //TODO: remove me

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
    proto->Set(String::NewSymbol("rotate"),
               FunctionTemplate::New(Rotate)->GetFunction());
    proto->Set(String::NewSymbol("toGray"),
               FunctionTemplate::New(ToGray)->GetFunction());
    proto->Set(String::NewSymbol("otsuAdaptiveThreshold"),
               FunctionTemplate::New(OtsuAdaptiveThreshold)->GetFunction());
    proto->Set(String::NewSymbol("findSkew"),
               FunctionTemplate::New(FindSkew)->GetFunction());
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
        // Copy to leptonica PIX.
        if (state.info_png.color.colortype == LCT_GREY || state.info_png.color.colortype == LCT_GREY_ALPHA) {
            pix = pixCreateNoInit(width, height, 8);
        } else {
            pix = pixCreateNoInit(width, height, 32);
        }
        std::vector<unsigned char>::const_iterator iter = out.begin();
        uint32_t *line = pix->data;
        for (uint32_t y = 0; y < pix->h; ++y) {
            for (uint32_t x = 0; x < pix->w; ++x) {
                if (pix->d == 8) {
                    // Fetch Gray = Red = Green = Blue.
                    SET_DATA_BYTE(line, x, *iter);
                    // Skip Green, Blue and Alpha.
                    ++iter; ++iter; ++iter; ++iter;
                } else {
                    // Fetch RGB.
                    int rval = *iter; ++iter;
                    int gval = *iter; ++iter;
                    int bval = *iter; ++iter;
                    composeRGBPixel(rval, gval, bval, &line[x]);
                    // Skip Alpha.
                    ++iter;
                }
            }
            line += pix->wpl;
        }
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

Handle<Value> Image::Rotate(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 1 && args[0]->IsNumber()) {
        const float deg2rad = 3.1415926535 / 180.;
        float angle = args[0]->ToNumber()->Value();
        std::cout << pixGetColormap(obj->pix_) << std::endl;
        Pix *pixd = pixRotateAM(obj->pix_, deg2rad * angle, L_BRING_IN_WHITE);
        if (pixd == NULL) {
            return THROW(TypeError, "error while rotating");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected number as first argument");
    }
}

Handle<Value> Image::ToGray(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (obj->pix_->d <= 8) {
        return scope.Close(Image::New(pixClone(obj->pix_)));
    }
    if (args.Length() == 3) {
        if (args[0]->IsNumber() && args[1]->IsNumber() && args[2]->IsNumber()) {
            float rwt = args[0]->ToNumber()->Value();
            float gwt = args[1]->ToNumber()->Value();
            float bwt = args[2]->ToNumber()->Value();
            PIX *grayPix = pixConvertRGBToGray(
                        obj->pix_, rwt, gwt, bwt);
            if (grayPix != NULL) {
                return scope.Close(Image::New(grayPix));
            } else {
                return THROW(Error, "error while computing grayscale image");
            }
        } else {
            return THROW(TypeError, "expected (int, int, int, int, float) signature");
        }
    } else if (args.Length() == 1) {
        if (args[0]->IsString()) {
            String::AsciiValue type(args[0]->ToString());
            int32_t typeInt;
            if (strcmp("min", *type) != 0) {
                typeInt = L_CHOOSE_MIN;
            } else if (strcmp("max", *type) != 0) {
                typeInt = L_CHOOSE_MAX;
            } else {
                return THROW(Error, "expected type to be 'min' or 'max'");
            }
            PIX *grayPix = pixConvertRGBToGrayMinMax(
                        obj->pix_, typeInt);
            if (grayPix != NULL) {
                return scope.Close(Image::New(grayPix));
            } else {
                return THROW(Error, "error while computing grayscale image");
            }
        } else {
            return THROW(TypeError, "expected (string) signature");
        }
    } else {
        return THROW(TypeError, "could not convert arguments");
    }
}

Handle<Value> Image::OtsuAdaptiveThreshold(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 5) {
        if (args[0]->IsInt32() && args[1]->IsInt32()
                && args[2]->IsInt32() && args[3]->IsInt32()
                && args[4]->IsNumber()) {
            int32_t sx = args[0]->ToInt32()->Value();
            int32_t sy = args[1]->ToInt32()->Value();
            int32_t smoothx = args[2]->ToInt32()->Value();
            int32_t smoothy = args[3]->ToInt32()->Value();
            float scorefact = args[4]->ToNumber()->Value();
            PIX *ppixth;
            PIX *ppixd;
            int error = pixOtsuAdaptiveThreshold(
                        obj->pix_, sx, sy, smoothx, smoothy,
                        scorefact, &ppixth, &ppixd);
            if (error == 0) {
                Local<Object> object = Object::New();
                object->Set(String::NewSymbol("thresholdValues"), Image::New(ppixth));
                object->Set(String::NewSymbol("image"), Image::New(ppixd));
                return scope.Close(object);
            } else {
                return THROW(Error, "error while computing threshold");
            }
        } else {
            return THROW(TypeError, "expected (int, int, int, int, float) signature");
        }
    } else {
        return THROW(TypeError, "could not convert arguments");
    }
}

Handle<Value> Image::FindSkew(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    int32_t depth = pixGetDepth(obj->pix_);
    if (depth != 1) {
        return THROW(TypeError, "expected binarized image");
    }
    float angle;
    float conf;
    int error = pixFindSkew(obj->pix_, &angle, &conf);
    if (error == 0) {
        Local<Object> object = Object::New();
        object->Set(String::NewSymbol("angle"), Number::New(angle));
        object->Set(String::NewSymbol("confidence"), Number::New(conf));
        return scope.Close(object);
    } else {
        return THROW(Error, "angle measurment not valid");
    }
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
        lodepng::State state;
        unsigned error;
        if (obj->pix_->d == 32 || obj->pix_->d == 24) {
            // Image is RGB, so create a 3 byte per pixel image.
            uint32_t *line;
            std::vector<unsigned char> in;
            in.reserve(obj->pix_->w * obj->pix_->h * 3);
            line = obj->pix_->data;
            for (uint32_t y = 0; y < obj->pix_->h; ++y) {
                for (uint32_t x = 0; x < obj->pix_->w; ++x) {
                    int32_t rval, gval, bval;
                    extractRGBValues(line[x], &rval, &gval, &bval);
                    in.push_back(rval);
                    in.push_back(gval);
                    in.push_back(bval);
                }
                line += obj->pix_->wpl;
            }
            state.info_png.color.colortype = LCT_RGB;
            state.info_raw.colortype = LCT_RGB;
            error = lodepng::encode(out, in, obj->pix_->w, obj->pix_->h, state);
        } else {
            // Image is Grayscale, so create a 1 byte per pixel image.
            uint32_t *line;
            std::vector<unsigned char> in;
            in.reserve(obj->pix_->w * obj->pix_->h);
            line = obj->pix_->data;
            for (uint32_t y = 0; y < obj->pix_->h; ++y) {
                for (uint32_t x = 0; x < obj->pix_->w; ++x) {
                    in.push_back(GET_DATA_BYTE(line, x));
                }
                line += obj->pix_->wpl;
            }
            state.info_png.color.colortype = LCT_GREY;
            state.info_raw.colortype = LCT_GREY;
            error = lodepng::encode(out, in, obj->pix_->w, obj->pix_->h, state);
        }
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
