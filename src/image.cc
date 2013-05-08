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
#include "image.h"
#include "util.h"
#include <cmath>
#include <node_buffer.h>
#include <jpgd.h>
#include <lodepng.h>
#include <sstream>

using namespace v8;
using namespace node;

Persistent<FunctionTemplate> Image::constructor_template;

Pix* pixFromSource(uint8_t *pixSource, int32_t width, int32_t height, int32_t depth, int32_t targetDepth)
{
    // Create PIX and copy pixels from source.
    PIX *pix = pixCreateNoInit(width, height, targetDepth);
    uint32_t *line = pix->data;
    for (uint32_t y = 0; y < pix->h; ++y) {
        for (uint32_t x = 0; x < pix->w; ++x) {
            if (pix->d == 8) {
                SET_DATA_BYTE(line, x, pixSource[0]);
            } else {
                composeRGBPixel(pixSource[0], pixSource[1], pixSource[2], &line[x]);
            }
            pixSource += depth / 8;
        }
        line += pix->wpl;
    }
    return pix;
}

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
    proto->SetAccessor(String::NewSymbol("width"), GetWidth);
    proto->SetAccessor(String::NewSymbol("height"), GetHeight);
    proto->SetAccessor(String::NewSymbol("depth"), GetDepth);
    proto->Set(String::NewSymbol("invert"),
               FunctionTemplate::New(Invert)->GetFunction());
    proto->Set(String::NewSymbol("or"),
               FunctionTemplate::New(Or)->GetFunction());
    proto->Set(String::NewSymbol("and"),
               FunctionTemplate::New(And)->GetFunction());
    proto->Set(String::NewSymbol("xor"),
               FunctionTemplate::New(Xor)->GetFunction());
    proto->Set(String::NewSymbol("subtract"),
               FunctionTemplate::New(Subtract)->GetFunction());
    proto->Set(String::NewSymbol("convolve"),
               FunctionTemplate::New(Convolve)->GetFunction());
    proto->Set(String::NewSymbol("rotate"),
               FunctionTemplate::New(Rotate)->GetFunction());
    proto->Set(String::NewSymbol("scale"),
               FunctionTemplate::New(Scale)->GetFunction());
    proto->Set(String::NewSymbol("crop"),
               FunctionTemplate::New(Crop)->GetFunction());
    proto->Set(String::NewSymbol("histogram"),
               FunctionTemplate::New(Histogram)->GetFunction());
    proto->Set(String::NewSymbol("setMasked"),
               FunctionTemplate::New(SetMasked)->GetFunction());
    proto->Set(String::NewSymbol("applyCurve"),
               FunctionTemplate::New(ApplyCurve)->GetFunction());
    proto->Set(String::NewSymbol("rankFilter"),
               FunctionTemplate::New(RankFilter)->GetFunction());
    proto->Set(String::NewSymbol("threshold"),
               FunctionTemplate::New(Threshold)->GetFunction());
    proto->Set(String::NewSymbol("toGray"),
               FunctionTemplate::New(ToGray)->GetFunction());
    proto->Set(String::NewSymbol("erode"),
               FunctionTemplate::New(Erode)->GetFunction());
    proto->Set(String::NewSymbol("dilate"),
               FunctionTemplate::New(Dilate)->GetFunction());
    proto->Set(String::NewSymbol("thin"),
               FunctionTemplate::New(Thin)->GetFunction());
    proto->Set(String::NewSymbol("maxDynamicRange"),
               FunctionTemplate::New(MaxDynamicRange)->GetFunction());
    proto->Set(String::NewSymbol("otsuAdaptiveThreshold"),
               FunctionTemplate::New(OtsuAdaptiveThreshold)->GetFunction());
    proto->Set(String::NewSymbol("findSkew"),
               FunctionTemplate::New(FindSkew)->GetFunction());
    proto->Set(String::NewSymbol("connectedComponents"),
               FunctionTemplate::New(ConnectedComponents)->GetFunction());
    proto->Set(String::NewSymbol("distanceFunction"),
               FunctionTemplate::New(DistanceFunction)->GetFunction());
    proto->Set(String::NewSymbol("clearBox"),
               FunctionTemplate::New(ClearBox)->GetFunction());
    proto->Set(String::NewSymbol("drawBox"),
               FunctionTemplate::New(DrawBox)->GetFunction());
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
    } else if (args.Length() == 1 &&  Image::HasInstance(args[0])) {
        pix = pixCopy(NULL, Image::Pixels(args[0]->ToObject()));
    } else if (args.Length() == 2 && Buffer::HasInstance(args[1])) {
        String::AsciiValue format(args[0]->ToString());
        Local<Object> buffer = args[1]->ToObject();
        unsigned char *in = reinterpret_cast<unsigned char*>(Buffer::Data(buffer));
        size_t inLength = Buffer::Length(buffer);
        if (strcmp("png", *format) == 0) {
            std::vector<unsigned char> out;
            unsigned int width;
            unsigned int height;
            lodepng::State state;
            unsigned error = lodepng::decode(out, width, height, state, in, inLength);
            if (error) {
                std::stringstream msg;
                msg << "error while decoding '" << lodepng_error_text(error) << "'";
                return THROW(Error, msg.str().c_str());
            }
            if (state.info_png.color.colortype == LCT_GREY || state.info_png.color.colortype == LCT_GREY_ALPHA) {
                pix = pixFromSource(&out[0], width, height, 32, 8);
            } else {
                pix = pixFromSource(&out[0], width, height, 32, 32);
            }
        } else if (strcmp("jpg", *format) == 0) {
            int width;
            int height;
            int comps;
            unsigned char *out = jpgd::decompress_jpeg_image_from_memory(
                        in, static_cast<int>(inLength), &width, &height, &comps, 4);
            if (!out) {
                return THROW(Error, "error while decoding jpg");
            }
            pix = pixFromSource(out, width, height, 32, comps == 1 ? 8 : 32);
            free(out);
        } else {
            std::stringstream msg;
            msg << "invalid bufffer format '" << *format << "'";
            return THROW(Error, msg.str().c_str());
        }
    } else if (args.Length() == 4 && Buffer::HasInstance(args[1])) {
        String::AsciiValue format(args[0]->ToString());
        Local<Object> buffer = args[1]->ToObject();
        size_t length = Buffer::Length(buffer);
        int32_t width = args[2]->Int32Value();
        int32_t height = args[3]->Int32Value();
        int32_t depth;
        if (strcmp("rgba", *format) == 0) {
            depth = 32;
        } else if (strcmp("rgb", *format) == 0) {
            depth = 24;
        } else if (strcmp("gray", *format) == 0) {
            depth = 8;
        } else {
            std::stringstream msg;
            msg << "invalid buffer format '" << *format << "'";
            return THROW(Error, msg.str().c_str());
        }
        size_t expectedLength = width * height * depth;
        if (expectedLength != length << 3) {
            return THROW(Error, "invalid Buffer length");
        }
        pix = pixFromSource(reinterpret_cast<uint8_t*>(Buffer::Data(buffer)), width, height, depth, 32);
    } else {
        return THROW(TypeError, "could not convert arguments");
    }
    Image* obj = new Image(pix);
    obj->Wrap(args.This());
    return args.This();
}

Handle<Value> Image::GetWidth(Local<String> prop, const AccessorInfo &info)
{
    Image *obj = ObjectWrap::Unwrap<Image>(info.This());
    return Number::New(obj->pix_->w);
}

Handle<Value> Image::GetHeight(Local<String> prop, const AccessorInfo &info)
{
    Image *obj = ObjectWrap::Unwrap<Image>(info.This());
    return Number::New(obj->pix_->h);
}

Handle<Value> Image::GetDepth(Local<String> prop, const AccessorInfo &info)
{
    Image *obj = ObjectWrap::Unwrap<Image>(info.This());
    return Number::New(obj->pix_->d);
}

Handle<Value> Image::Invert(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    Pix *pixd = pixInvert(NULL, obj->pix_);
    if (pixd == NULL) {
        return THROW(TypeError, "error while applying INVERT");
    }
    return scope.Close(Image::New(pixd));
}

Handle<Value> Image::Or(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 1 && Image::HasInstance(args[0])) {
        Pix *otherPix = Image::Pixels(args[0]->ToObject());
        Pix *pixd = pixOr(NULL, obj->pix_, otherPix);
        if (pixd == NULL) {
            return THROW(TypeError, "error while applying OR");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected image as first argument");
    }
}

Handle<Value> Image::And(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 1 && Image::HasInstance(args[0])) {
        Pix *otherPix = Image::Pixels(args[0]->ToObject());
        Pix *pixd = pixAnd(NULL, obj->pix_, otherPix);
        if (pixd == NULL) {
            return THROW(TypeError, "error while applying AND");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected image as first argument");
    }
}

Handle<Value> Image::Xor(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 1 && Image::HasInstance(args[0])) {
        Pix *otherPix = Image::Pixels(args[0]->ToObject());
        Pix *pixd = pixXor(NULL, obj->pix_, otherPix);
        if (pixd == NULL) {
            return THROW(TypeError, "error while applying XOR");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected image as first argument");
    }
}

Handle<Value> Image::Subtract(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 1 && Image::HasInstance(args[0])) {
        Pix *otherPix = Image::Pixels(args[0]->ToObject());
        Pix *pixd;
        if(obj->pix_->d >= 8) {
            pixd = pixSubtractGray(NULL, obj->pix_, otherPix);
        } else {
            pixd = pixSubtract(NULL, obj->pix_, otherPix);
        }
        if (pixd == NULL) {
            return THROW(TypeError, "error while applying SUBTRACT");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected image as first argument");
    }
}

Handle<Value> Image::Convolve(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32()) {
        int width = args[0]->Int32Value();
        int height = args[1]->Int32Value();
        Pix *pixs;
        if(obj->pix_->d == 1) {
            pixs = pixConvert1To8(NULL, obj->pix_, 0, 255);
        } else {
            pixs = pixClone(obj->pix_);
        }
        Pix *pixd = pixBlockconv(pixs, width, height);
        pixDestroy(&pixs);
        if (pixd == NULL) {
            return THROW(TypeError, "error while applying convolve");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (int, int) signature");
    }
}

Handle<Value> Image::Rotate(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 1 && args[0]->IsNumber()) {
        const float deg2rad = 3.1415926535 / 180.;
        float angle = args[0]->ToNumber()->Value();
        Pix *pixd = pixRotate(obj->pix_, deg2rad * angle,
                              L_ROTATE_AREA_MAP, L_BRING_IN_WHITE,
                              obj->pix_->w, obj->pix_->h);
        if (pixd == NULL) {
            return THROW(TypeError, "error while rotating");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected number as first argument");
    }
}

Handle<Value> Image::Scale(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if ((args.Length() >= 1 && args[0]->IsNumber()) || (args.Length() == 2 && args[1]->IsNumber())) {
        float scaleX = args[0]->ToNumber()->Value();
        float scaleY = args.Length() == 2 ? args[1]->ToNumber()->Value() : scaleX;
        Pix *pixd = pixScale(obj->pix_, scaleX, scaleY);
        if (pixd == NULL) {
            return THROW(TypeError, "error while scaling");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (float, [float]) signature");
    }
}

Handle<Value> Image::Crop(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 4 && args[0]->IsNumber()
            && args[1]->IsNumber() && args[2]->IsNumber()
            && args[3]->IsNumber()) {
        int left = floor(args[0]->ToNumber()->Value());
        int top = floor(args[1]->ToNumber()->Value());
        int width = ceil(args[2]->ToNumber()->Value());
        int height = ceil(args[3]->ToNumber()->Value());
        BOX *box = boxCreate(left, top, width, height);
        PIX *pixd = pixClipRectangle(obj->pix_, box, 0);
        boxDestroy(&box);
        if (pixd == NULL) {
            return THROW(TypeError, "error while cropping");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (int, int, int, int) signature");
    }
}

Handle<Value> Image::Histogram(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 0) {
        PIX *mask = NULL;
        if (args.Length() >= 1 && Image::HasInstance(args[0]))
            mask = Image::Pixels(args[1]->ToObject());

        NUMA *hist = pixGetGrayHistogramMasked(obj->pix_, mask, 0, 0, obj->pix_->h > 400 ? 2 : 1);
        
        if (!hist)
            return THROW(TypeError, "pixGetGrayHistogram failed");

        int len = numaGetCount(hist);
        unsigned int count = 0;
        for (int i = 0; i < len; i++) {
            count += hist->array[i];
        }
        
        Local<Array> result = Array::New(len);
        for (int i = 0; i < len; i++)
            result->Set(i, Number::New(hist->array[i] / count));
        
        numaDestroy(&hist);
        
        return scope.Close(result);
    } else {
        return THROW(TypeError, "expected no arguments");
    }
}

Handle<Value> Image::SetMasked(const Arguments &args)
{
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 2 && Image::HasInstance(args[0]) &&
            args[1]->IsNumber()) {
        Pix *mask = Image::Pixels(args[0]->ToObject());
        int value = floor(args[1]->ToNumber()->Value());
        if (pixSetMasked(obj->pix_, mask, value) == 1) {
            return THROW(TypeError, "error while cropping");
        }
        return args.This();
    } else {
        return THROW(TypeError, "expected (int, image) signature");
    }
}

Handle<Value> Image::ApplyCurve(const Arguments &args)
{
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() >= 1 && args[0]->IsArray() &&
            args[0]->ToObject()->Get(String::New("length"))->Uint32Value() == 256) {
        NUMA *numa = numaCreate(256);
        for (int i = 0; i < 256; i++) {
            numaAddNumber(numa, args[0]->ToObject()->Get(i)->ToInt32()->Value());
        }
        PIX *mask = NULL;
        if (args.Length() >= 2 && Image::HasInstance(args[1])) {
            mask = Image::Pixels(args[1]->ToObject());
        }
        int result = pixTRCMap(obj->pix_, mask, numa);
        if (result != 0) {
            return THROW(TypeError, "error while applying value mapping");
        }
        return args.This();
    } else {
        return THROW(TypeError, "expected (array[256] of int) signature");
    }
}

Handle<Value> Image::RankFilter(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 3 && args[0]->IsInt32() &&
            args[1]->IsInt32() && args[2]->IsNumber()) {
        int width = args[0]->ToInt32()->Value();
        int height = args[1]->ToInt32()->Value();
        float rank = args[2]->ToNumber()->Value();
        PIX *pixd = pixRankFilter(obj->pix_, width, height, rank);
        if (pixd == NULL) {
            return THROW(TypeError, "error while applying rank filter");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (int, int, float) signature");
    }
}

Handle<Value> Image::Threshold(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 0 || args[0]->IsInt32()) {
        int value = 128;
        if (args.Length() >= 1)
            value = args[0]->ToInt32()->Value();

        PIX *pixd = pixConvertTo1(obj->pix_, value);
        if (pixd == NULL) {
            return THROW(TypeError, "error while thresholding");
        }

        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (int, int, float) signature");
    }
}

Handle<Value> Image::ToGray(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (obj->pix_->d == 8) {
        return scope.Close(Image::New(pixClone(obj->pix_)));
    }
    if (args.Length() == 0) {
        PIX *grayPix = pixConvertTo8(obj->pix_, 0);
        if (grayPix != NULL) {
            return scope.Close(Image::New(grayPix));
        } else {
            return THROW(Error, "error while computing grayscale image");
        }
    } else if (args.Length() == 3) {
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
            if (strcmp("min", *type) == 0) {
                typeInt = L_CHOOSE_MIN;
            } else if (strcmp("max", *type) == 0) {
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

Handle<Value> Image::ClearBox(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 4
            && args[0]->IsNumber() && args[1]->IsNumber()
            && args[2]->IsNumber() && args[3]->IsNumber()) {
        int left = floor(args[0]->ToNumber()->Value());
        int top = floor(args[1]->ToNumber()->Value());
        int width = ceil(args[2]->ToNumber()->Value());
        int height = ceil(args[3]->ToNumber()->Value());
        BOX *box = boxCreate(left, top, width, height);
        int error;
        if (obj->pix_->d == 1) {
            error = pixClearInRect(obj->pix_, box);
        } else {
            error = pixSetInRect(obj->pix_, box);
        }
        boxDestroy(&box);
        if (error) {
            return THROW(TypeError, "error while clearing box");
        }
        return args.This();
    } else {
        return THROW(TypeError, "expected (int, int, int, int) signature");
    }
}

Handle<Value> Image::Erode(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32()) {
        int width = args[0]->ToInt32()->Value();
        int height = args[1]->ToInt32()->Value();
        PIX *pixd = 0;
        if (obj->pix_->d == 1) {
            pixd = pixErodeBrick(NULL, obj->pix_, width, height);
        } else {
            pixd = pixErodeGray(obj->pix_, width, height);
        }
        if (pixd == NULL) {
            return THROW(TypeError, "error while eroding");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (int, int) signature");
    }
}

Handle<Value> Image::Dilate(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32()) {
        int width = args[0]->ToInt32()->Value();
        int height = args[1]->ToInt32()->Value();
        PIX *pixd = 0;
        if (obj->pix_->d == 1) {
            pixd = pixDilateBrick(NULL, obj->pix_, width, height);
        } else {
            pixd = pixDilateGray(obj->pix_, width, height);
        }
        if (pixd == NULL) {
            return THROW(TypeError, "error while dilating");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (int, int) signature");
    }
}

Handle<Value> Image::Thin(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 3 && args[0]->IsString() &&
            args[1]->IsInt32() && args[2]->IsInt32()) {
        int typeInt = 0;
        String::AsciiValue type(args[0]->ToString());
        if (strcmp("fg", *type) == 0) {
            typeInt = L_THIN_FG;
        } else if (strcmp("bg", *type) == 0) {
            typeInt = L_THIN_BG;
        }
        int connectivity = args[1]->ToInt32()->Value();
        int maxIters = args[2]->ToInt32()->Value();
        PIX *pix = obj->pix_;
        // If image is grayscale, binarize with fixed threshold
        if (pix->d != 1) {
            pix = pixConvertTo1(pix, 128);
        }
        Pix *pixd = pixThin(pix, typeInt, connectivity, maxIters);
        if (pix != obj->pix_) {
            pixDestroy(&pix);
        }
        if (pixd == NULL) {
            return THROW(TypeError, "error while thinning");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (string, int, int) signature");
    }
}

Handle<Value> Image::MaxDynamicRange(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 1 && args[0]->IsString()) {
        int typeInt = 0;
        String::AsciiValue type(args[0]->ToString());
        if (strcmp("linear", *type) == 0) {
            typeInt = L_SET_PIXELS;
        } else if (strcmp("log", *type) == 0) {
            typeInt = L_LOG_SCALE;
        }
        PIX *pixd = pixMaxDynamicRange(obj->pix_, typeInt);
        if (pixd == NULL) {
            return THROW(TypeError, "error while computing max. dynamic range");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (string) signature");
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

Handle<Value> Image::ConnectedComponents(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 1 && args[0]->IsInt32()) {
        int connectivity = args[0]->ToInt32()->Value();
        PIX *pix = obj->pix_;
        // If image is grayscale, binarize with fixed threshold
        if (pix->d != 1) {
            pix = pixConvertTo1(pix, 128);
        }
        BOXA *boxa = pixConnCompBB(pix, connectivity);
        if (pix != obj->pix_) {
            pixDestroy(&pix);
        }
        if (!boxa) {
            return THROW(TypeError, "error while computing connected components");
        }
        Local<Object> boxes = Array::New();
        for (int i = 0; i < boxa->n; ++i) {
            boxes->Set(i, createBox(boxa->box[i]));
        }
        boxaDestroy(&boxa);
        return scope.Close(boxes);
    } else {
        return THROW(TypeError, "expected (int) signature");
    }
}

Handle<Value> Image::DistanceFunction(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 1 && args[0]->IsInt32()) {
        int connectivity = args[0]->ToInt32()->Value();
        PIX *pix = obj->pix_;
        // If image is grayscale, binarize with fixed threshold
        if (pix->d != 1) {
            pix = pixConvertTo1(pix, 128);
        }
        Pix *pixDistance = pixDistanceFunction(pix, connectivity, 8, L_BOUNDARY_BG);
        if (pix != obj->pix_) {
            pixDestroy(&pix);
        }
        if (pixDistance == NULL) {
            return THROW(TypeError, "error while computing distance function");
        }
        return scope.Close(Image::New(pixDistance));
    } else {
        return THROW(TypeError, "expected (int) signature");
    }
}

Handle<Value> Image::DrawBox(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if ((args.Length() == 5 || args.Length() == 6)
            && args[0]->IsNumber() && args[1]->IsNumber()
            && args[2]->IsNumber() && args[3]->IsNumber()
            && args[4]->IsInt32()) {
        int left = floor(args[0]->ToNumber()->Value());
        int top = floor(args[1]->ToNumber()->Value());
        int width = ceil(args[2]->ToNumber()->Value());
        int height = ceil(args[3]->ToNumber()->Value());
        BOX *box = boxCreate(left, top, width, height);
        int borderWidth = args[4]->ToInt32()->Value();
        int opInt = L_SET_PIXELS;
        if (args.Length() == 6 && args[5]->IsString()) {
            String::AsciiValue op(args[5]->ToString());
            if (strcmp("set", *op) == 0) {
                opInt = L_SET_PIXELS;
            } else if (strcmp("clear", *op) == 0) {
                opInt = L_CLEAR_PIXELS;
            } else if (strcmp("flip", *op) == 0) {
                opInt = L_FLIP_PIXELS;
            }
        }
        int error = pixRenderBox(obj->pix_, box, borderWidth, opInt);
        boxDestroy(&box);
        if (error) {
            return THROW(TypeError, "error while drawing box");
        }
        return args.This();
    } else {
        return THROW(TypeError, "expected (int, int, int, int, int[, string]) signature");
    }
}

Handle<Value> Image::ToBuffer(const Arguments &args)
{
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    bool encodePNG = false;
    if (args.Length() == 1 && args[0]->IsString()) {
        String::AsciiValue format(args[0]->ToString());
        if (strcmp("png", *format) != 0) {
            std::stringstream msg;
            msg << "invalid bufffer format '" << *format << "'";
            return THROW(Error, msg.str().c_str());
        }
        encodePNG = true;
    }
    if (args.Length() <= 1) {
        std::vector<unsigned char> imgData;
        std::vector<unsigned char> pngData;
        lodepng::State state;
        unsigned error = 0;
        if (obj->pix_->d == 32 || obj->pix_->d == 24) {
            // Image is RGB, so create a 3 byte per pixel image.
            uint32_t *line;
            imgData.reserve(obj->pix_->w * obj->pix_->h * 3);
            line = obj->pix_->data;
            for (uint32_t y = 0; y < obj->pix_->h; ++y) {
                for (uint32_t x = 0; x < obj->pix_->w; ++x) {
                    int32_t rval, gval, bval;
                    extractRGBValues(line[x], &rval, &gval, &bval);
                    imgData.push_back(rval);
                    imgData.push_back(gval);
                    imgData.push_back(bval);
                }
                line += obj->pix_->wpl;
            }
            if (encodePNG) {
                state.info_png.color.colortype = LCT_RGB;
                state.info_raw.colortype = LCT_RGB;
                error = lodepng::encode(pngData, imgData, obj->pix_->w, obj->pix_->h, state);
            }
        } else if (obj->pix_->d <= 8) {
            PIX *pix8 = pixConvertTo8(obj->pix_, 0);
            // Image is Grayscale, so create a 1 byte per pixel image.
            uint32_t *line;
            imgData.reserve(pix8->w * pix8->h);
            line = pix8->data;
            for (uint32_t y = 0; y < pix8->h; ++y) {
                for (uint32_t x = 0; x < pix8->w; ++x) {
                    imgData.push_back(GET_DATA_BYTE(line, x));
                }
                line += pix8->wpl;
            }
            if (encodePNG) {
                state.info_png.color.colortype = LCT_GREY;
                state.info_raw.colortype = LCT_GREY;
                error = lodepng::encode(pngData, imgData, pix8->w, pix8->h, state);
            }
            pixDestroy(&pix8);
        } else {
            return THROW(Error, "wrong image format");
        }
        if (error) {
            std::stringstream msg;
            msg << "error while encoding '" << lodepng_error_text(error) << "'";
            return THROW(Error, msg.str().c_str());
        }
        if (encodePNG)
            return Buffer::New(reinterpret_cast<char *>(&pngData[0]), pngData.size())->handle_;
        else
            return Buffer::New(reinterpret_cast<char *>(&imgData[0]), imgData.size())->handle_;
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
        return pix_->h * pix_->wpl * sizeof(uint32_t);
    } else {
        return 0;
    }
}
