/*
 * node-dv - Document Vision for node.js
 *
 * Copyright (c) 2012 Christoph Schulz
 * Copyright (c) 2013-2015 creatale GmbH, contributors listed under AUTHORS
 * 
 * MIT License <https://github.com/creatale/node-dv/blob/master/LICENSE>
 */
#include "image.h"
#include "util.h"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <node_buffer.h>
#include <lodepng.h>
#include <jpgd.h>
#include <jpge.h>
#include <opencv2/core/core.hpp>
#include <LSWMS.h>

#ifdef _MSC_VER
#if _MSC_VER <= 1700
int round(double x)
{
    return (x > 0.0) ? std::floor(x + 0.5) : std::floor(x - 0.5);
}
#endif
#endif

using namespace v8;
using namespace node;

namespace binding {

Persistent<FunctionTemplate> Image::constructor_template;

PIX *pixFromSource(uint8_t *pixSource, int32_t width, int32_t height, int32_t depth, int32_t targetDepth)
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

PIX *pixInRange(PIX *pixs, l_int32 val1l, l_int32 val2l, l_int32 val3l, l_int32 val1u, l_int32 val2u, l_int32 val3u)
{
    if (!pixs || pixGetDepth(pixs) != 32) {
        return NULL;
    }

    l_int32 w, h;
    pixGetDimensions(pixs, &w, &h, NULL);
    PIX *pixd = pixCreate(w, h, 1);
    l_uint32 *datas = pixGetData(pixs);
    l_uint32 *datad = pixGetData(pixd);
    l_int32 wpls = pixGetWpl(pixs);
    l_int32 wpld = pixGetWpl(pixd);
    for (l_int32 i = 0; i < h; i++) {
        l_uint32 *lines = datas + i * wpls;
        l_uint32 *lined = datad + i * wpld;
        for (l_int32 j = 0; j < w; j++) {
            l_uint32 pixel = lines[j];
            l_int32 val1 = (pixel >> L_RED_SHIFT) & 0xff;
            l_int32 val2 = (pixel >> L_GREEN_SHIFT) & 0xff;
            l_int32 val3 = (pixel >> L_BLUE_SHIFT) & 0xff;
            if (val1 >= val1l && val1 <= val1u &&
                    val2 >= val2l && val2 <= val2u &&
                    val3 >= val3l && val3 <= val3u) {
                SET_DATA_BIT(lined, j);
            }
        }
    }

    return pixd;
}

PIX *pixCompose(PIX *pix1, PIX *pix2, PIX *pix3)
{
    if ((!pix1 && pixGetDepth(pix1) != 8) ||
            (!pix2 && pixGetDepth(pix2) != 8) ||
            (!pix3 && pixGetDepth(pix3) != 8)) {
        return NULL;
    }

    l_int32 w1, h1, w2, h2, w3, h3;
    pixGetDimensions(pix1, &w1, &h1, NULL);
    pixGetDimensions(pix2, &w2, &h2, NULL);
    pixGetDimensions(pix3, &w3, &h3, NULL);
    l_int32 w = std::min(std::min(w1, w2), w3);
    l_int32 h = std::min(std::min(h1, h2), h3);
    PIX *pixd = pixCreate(w, h, 32);
    l_uint32 *data1 = pixGetData(pix1);
    l_uint32 *data2 = pixGetData(pix2);
    l_uint32 *data3 = pixGetData(pix3);
    l_uint32 *datad = pixGetData(pixd);
    l_int32 wpl1 = pixGetWpl(pix1);
    l_int32 wpl2 = pixGetWpl(pix2);
    l_int32 wpl3 = pixGetWpl(pix3);
    l_int32 wpld = pixGetWpl(pixd);
    for (l_int32 i = 0; i < h; i++) {
        l_uint32 *line1 = data1 + i * wpl1;
        l_uint32 *line2 = data2 + i * wpl2;
        l_uint32 *line3 = data3 + i * wpl3;
        l_uint32 *lined = datad + i * wpld;
        for (l_int32 j = 0; j < w; j++) {
            l_int32 val1 = GET_DATA_BYTE(line1, j);
            l_int32 val2 = GET_DATA_BYTE(line2, j);
            l_int32 val3 = GET_DATA_BYTE(line3, j);
            composeRGBPixel(val1, val2, val3, &lined[j]);
        }
    }

    return pixd;
}

enum ProjectionMode
{
    Horizontal = 1,
    Vertical = 2
};

void pixProjection(std::vector<uint32_t> &values, PIX *pix, ProjectionMode mode)
{
    uint32_t *line = pix->data;
    if (mode == Horizontal) {
        values.assign(pix->w, 0);
    }
    else if (mode == Vertical) {
        values.assign(pix->h, 0);
    }
    for (uint32_t y = 0; y < pix->h; ++y) {
        for (uint32_t x = 0; x < pix->w; ++x) {
            if (mode == Horizontal) {
                if (pix->d == 1) {
                    values[x] += GET_DATA_BIT(line, x);
                }
                else {
                    values[x] += GET_DATA_BYTE(line, x);
                }
            }
            else if (mode == Vertical) {
                if (pix->d == 1) {
                    values[y] += GET_DATA_BIT(line, x);
                }
                else {
                    values[y] += GET_DATA_BYTE(line, x);
                }
            }
        }
        line += pix->wpl;
    }
}

cv::Mat pix8ToMat(PIX *pix8)
{
    cv::Mat mat(cv::Size(pix8->w, pix8->h), CV_8UC1);
    uint32_t *line = pix8->data;
    for (uint32_t y = 0; y < pix8->h; ++y) {
        for (uint32_t x = 0; x < pix8->w; ++x) {
            mat.at<uchar>(y, x) = GET_DATA_BYTE(line, x);
        }
        line += pix8->wpl;
    }
    return mat;
}

bool Image::HasInstance(Handle<Value> val)
{
    if (!val->IsObject()) {
        return false;
    }
    return constructor_template->HasInstance(val->ToObject());
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
    proto->Set(String::NewSymbol("add"),
               FunctionTemplate::New(Add)->GetFunction());
    proto->Set(String::NewSymbol("subtract"),
               FunctionTemplate::New(Subtract)->GetFunction());
    proto->Set(String::NewSymbol("convolve"),
               FunctionTemplate::New(Convolve)->GetFunction());
    proto->Set(String::NewSymbol("unsharpMasking"), //TODO: remove (deprecated).
               FunctionTemplate::New(Unsharp)->GetFunction());
    proto->Set(String::NewSymbol("unsharp"),
               FunctionTemplate::New(Unsharp)->GetFunction());
    proto->Set(String::NewSymbol("rotate"),
               FunctionTemplate::New(Rotate)->GetFunction());
    proto->Set(String::NewSymbol("scale"),
               FunctionTemplate::New(Scale)->GetFunction());
    proto->Set(String::NewSymbol("crop"),
               FunctionTemplate::New(Crop)->GetFunction());
    proto->Set(String::NewSymbol("inRange"),
               FunctionTemplate::New(InRange)->GetFunction());
    proto->Set(String::NewSymbol("histogram"),
               FunctionTemplate::New(Histogram)->GetFunction());
    proto->Set(String::NewSymbol("projection"),
               FunctionTemplate::New(Projection)->GetFunction());
    proto->Set(String::NewSymbol("setMasked"),
               FunctionTemplate::New(SetMasked)->GetFunction());
    proto->Set(String::NewSymbol("applyCurve"),
               FunctionTemplate::New(ApplyCurve)->GetFunction());
    proto->Set(String::NewSymbol("rankFilter"),
               FunctionTemplate::New(RankFilter)->GetFunction());
    proto->Set(String::NewSymbol("octreeColorQuant"),
               FunctionTemplate::New(OctreeColorQuant)->GetFunction());
    proto->Set(String::NewSymbol("medianCutQuant"),
               FunctionTemplate::New(MedianCutQuant)->GetFunction());
    proto->Set(String::NewSymbol("threshold"),
               FunctionTemplate::New(Threshold)->GetFunction());
    proto->Set(String::NewSymbol("toGray"),
               FunctionTemplate::New(ToGray)->GetFunction());
    proto->Set(String::NewSymbol("toColor"),
               FunctionTemplate::New(ToColor)->GetFunction());
    proto->Set(String::NewSymbol("toHSV"),
               FunctionTemplate::New(ToHSV)->GetFunction());
    proto->Set(String::NewSymbol("toRGB"),
               FunctionTemplate::New(ToRGB)->GetFunction());
    proto->Set(String::NewSymbol("erode"),
               FunctionTemplate::New(Erode)->GetFunction());
    proto->Set(String::NewSymbol("dilate"),
               FunctionTemplate::New(Dilate)->GetFunction());
    proto->Set(String::NewSymbol("open"),
               FunctionTemplate::New(Open)->GetFunction());
    proto->Set(String::NewSymbol("close"),
               FunctionTemplate::New(Close)->GetFunction());
    proto->Set(String::NewSymbol("thin"),
               FunctionTemplate::New(Thin)->GetFunction());
    proto->Set(String::NewSymbol("maxDynamicRange"),
               FunctionTemplate::New(MaxDynamicRange)->GetFunction());
    proto->Set(String::NewSymbol("otsuAdaptiveThreshold"),
               FunctionTemplate::New(OtsuAdaptiveThreshold)->GetFunction());
    proto->Set(String::NewSymbol("lineSegments"),
               FunctionTemplate::New(LineSegments)->GetFunction());
    proto->Set(String::NewSymbol("findSkew"),
               FunctionTemplate::New(FindSkew)->GetFunction());
    proto->Set(String::NewSymbol("connectedComponents"),
               FunctionTemplate::New(ConnectedComponents)->GetFunction());
    proto->Set(String::NewSymbol("distanceFunction"),
               FunctionTemplate::New(DistanceFunction)->GetFunction());
    proto->Set(String::NewSymbol("clearBox"), //TODO: remove (deprecated).
               FunctionTemplate::New(ClearBox)->GetFunction());
    proto->Set(String::NewSymbol("fillBox"),
               FunctionTemplate::New(FillBox)->GetFunction());
    proto->Set(String::NewSymbol("drawBox"),
               FunctionTemplate::New(DrawBox)->GetFunction());
    proto->Set(String::NewSymbol("drawImage"),
               FunctionTemplate::New(DrawImage)->GetFunction());
    proto->Set(String::NewSymbol("drawLine"),
               FunctionTemplate::New(DrawLine)->GetFunction());
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
    if (obj->pix_) {
        V8::AdjustAmountOfExternalAllocatedMemory(obj->size());
    }
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
    } else if (args.Length() == 3 && args[0]->IsNumber() && args[1]->IsNumber()
               && args[2]->IsNumber()) {
        int32_t width = args[0]->Int32Value();
        int32_t height = args[1]->Int32Value();
        int32_t depth = args[2]->Int32Value();
        pix = pixCreate(width, height, depth);
    } else if (args.Length() == 3 &&
               (Image::HasInstance(args[0]) || args[0]->IsNull() || args[0]->IsUndefined()) &&
               (Image::HasInstance(args[1]) || args[1]->IsNull() || args[0]->IsUndefined()) &&
               (Image::HasInstance(args[2]) || args[2]->IsNull() || args[0]->IsUndefined())) {
        pix = pixCompose(Image::Pixels(args[0]->ToObject()),
                Image::Pixels(args[1]->ToObject()),
                Image::Pixels(args[2]->ToObject()));
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
        return THROW(TypeError, "expected (image: Image) or (image1: Image, "
                     "image2: Image, image3: Image) or (format: String, "
                     "image: Buffer, [width: Int32, height: Int32]) or no arguments at all");
    }
    Image* obj = new Image(pix);
    obj->Wrap(args.This());
    return args.This();
}

Handle<Value> Image::GetWidth(Local<String> prop, const AccessorInfo &info)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(info.This());
    return scope.Close(Number::New(obj->pix_->w));
}

Handle<Value> Image::GetHeight(Local<String> prop, const AccessorInfo &info)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(info.This());
    return scope.Close(Number::New(obj->pix_->h));
}

Handle<Value> Image::GetDepth(Local<String> prop, const AccessorInfo &info)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(info.This());
    return scope.Close(Number::New(obj->pix_->d));
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
    if (Image::HasInstance(args[0])) {
        Pix *otherPix = Image::Pixels(args[0]->ToObject());
        Pix *pixd = pixOr(NULL, obj->pix_, otherPix);
        if (pixd == NULL) {
            return THROW(TypeError, "error while applying OR");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (image: Image)");
    }
}

Handle<Value> Image::And(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (Image::HasInstance(args[0])) {
        Pix *otherPix = Image::Pixels(args[0]->ToObject());
        Pix *pixd = pixAnd(NULL, obj->pix_, otherPix);
        if (pixd == NULL) {
            return THROW(TypeError, "error while applying AND");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (image: Image)");
    }
}

Handle<Value> Image::Xor(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (Image::HasInstance(args[0])) {
        Pix *otherPix = Image::Pixels(args[0]->ToObject());
        Pix *pixd = pixXor(NULL, obj->pix_, otherPix);
        if (pixd == NULL) {
            return THROW(TypeError, "error while applying XOR");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (image: Image)");
    }
}

Handle<Value> Image::Add(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (Image::HasInstance(args[0])) {
        Pix *otherPix = Image::Pixels(args[0]->ToObject());
        Pix *pixd;
        if (obj->pix_->d == 32) {
            pixd = pixAddRGB(obj->pix_, otherPix);
        } else if (obj->pix_->d >= 8) {
            if (obj->pix_ == otherPix) {
                otherPix = pixCopy(NULL, otherPix);
                pixd = pixAddGray(NULL, obj->pix_, otherPix);
                pixDestroy(&otherPix);
            } else {
                pixd = pixAddGray(NULL, obj->pix_, otherPix);
            }
        } else {
            pixd = pixOr(NULL, obj->pix_, otherPix);
        }
        if (pixd == NULL) {
            return THROW(TypeError, "error while applying ADD");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (image: Image)");
    }
}

Handle<Value> Image::Subtract(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (Image::HasInstance(args[0])) {
        Pix *otherPix = Image::Pixels(args[0]->ToObject());
        Pix *pixd;
        if(obj->pix_->d >= 8) {
            if (obj->pix_ == otherPix) {
                otherPix = pixCopy(NULL, otherPix);
                pixd = pixSubtractGray(NULL, obj->pix_, otherPix);
                pixDestroy(&otherPix);
            } else {
                pixd = pixSubtractGray(NULL, obj->pix_, otherPix);
            }
        } else {
            pixd = pixSubtract(NULL, obj->pix_, otherPix);
        }
        if (pixd == NULL) {
            return THROW(TypeError, "error while applying SUBTRACT");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (image: Image)");
    }
}

Handle<Value> Image::Convolve(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsNumber() && args[1]->IsNumber()) {
        int width = static_cast<int>(ceil(args[0]->NumberValue()));
        int height = static_cast<int>(ceil(args[1]->NumberValue()));
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
        return THROW(TypeError, "expected (width: Number, height: Number)");
    }
}

Handle<Value> Image::Unsharp(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsNumber() && args[1]->IsNumber()) {
        int halfWidth = static_cast<int>(ceil(args[0]->NumberValue()));
        float fract = static_cast<float>(args[1]->NumberValue());
        Pix *pixd = pixUnsharpMasking(obj->pix_, halfWidth, fract);
        if (pixd == NULL) {
            return THROW(TypeError, "error while applying unsharp");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (halfWidth: Number, fract: Number)");
    }
}

Handle<Value> Image::Rotate(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsNumber()) {
        const float deg2rad = 3.1415926535f / 180.0f;
        float angle = static_cast<float>(args[0]->NumberValue());
        Pix *pixd = pixRotate(obj->pix_, deg2rad * angle,
                              L_ROTATE_AREA_MAP, L_BRING_IN_WHITE,
                              obj->pix_->w, obj->pix_->h);
        if (pixd == NULL) {
            return THROW(TypeError, "error while applying rotate");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (angle: Number)");
    }
}

Handle<Value> Image::Scale(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsNumber() && (args.Length() != 2 || args[1]->IsNumber())) {
        float scaleX = static_cast<float>(args[0]->NumberValue());
        float scaleY = static_cast<float>(args.Length() == 2 ? args[1]->NumberValue() : scaleX);
        Pix *pixd = pixScale(obj->pix_, scaleX, scaleY);
        if (pixd == NULL) {
            return THROW(TypeError, "error while scaling");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (scaleX: Number, [scaleY: Number])");
    }
}

Handle<Value> Image::Crop(const Arguments &args)
{
    HandleScope scope;
    Box* box = toBox(args, 0);
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (box) {
        PIX *pixd = pixClipRectangle(obj->pix_, box, 0);
        boxDestroy(&box);
        if (pixd == NULL) {
            return THROW(TypeError, "error while cropping");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (box: Box)");
    }
}

Handle<Value> Image::InRange(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsNumber() && args[1]->IsNumber() && args[2]->IsNumber() &&
            args[3]->IsNumber() && args[4]->IsNumber() && args[5]->IsNumber()) {
        int32_t val1l = args[0]->Int32Value();
        int32_t val2l = args[1]->Int32Value();
        int32_t val3l = args[2]->Int32Value();
        int32_t val1u = args[3]->Int32Value();
        int32_t val2u = args[4]->Int32Value();
        int32_t val3u = args[5]->Int32Value();
        Pix *pixd = pixInRange(obj->pix_, val1l, val2l, val3l,
                               val1u, val2u, val3u);
        if (pixd == NULL) {
            return THROW(TypeError, "error while computing in range mask");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected ("
                     "lower1: Number, lower2: Number, lower3: Number, "
                     "upper1: Number, upper2: Number, upper3: Number)");
    }
}

Handle<Value> Image::Histogram(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    PIX *mask = NULL;
    if (args.Length() >= 1 && Image::HasInstance(args[0])) {
        mask = Image::Pixels(args[1]->ToObject());
    }
    NUMA *hist = pixGetGrayHistogramMasked(obj->pix_, mask, 0, 0, obj->pix_->h > 400 ? 2 : 1);
    if (!hist) {
        return THROW(TypeError, "error while computing histogram");
    }
    int len = numaGetCount(hist);
    l_float32 total = 0;
    for (int i = 0; i < len; i++) {
        total += hist->array[i];
    }
    Local<Array> result = Array::New(len);
    for (int i = 0; i < len; i++) {
        result->Set(i, Number::New(hist->array[i] / total));
    }
    numaDestroy(&hist);
    return scope.Close(result);
}

Handle<Value> Image::Projection(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsString()) {
        String::AsciiValue mode(args[0]->ToString());
        ProjectionMode modeEnum;
        if (strcmp("horizontal", *mode) == 0) {
            modeEnum = Horizontal;
        }
        else if (strcmp("vertical", *mode) == 0) {
            modeEnum = Vertical;
        }
        else {
            return THROW(Error, "expected mode to be 'horizontal' or 'vertical'");
        }
        if (obj->pix_->d != 8 && obj->pix_->d != 1) {
            return THROW(Error, "expected 8bpp or 1bpp image");
        }
        std::vector<uint32_t> values;
        pixProjection(values, obj->pix_, modeEnum);
        Local<Array> result = Array::New(static_cast<int>(values.size()));
        for (int i = 0; i < values.size(); i++) {
            result->Set(i, Number::New(values[i]));
        }
        return scope.Close(result);
    }
    else {
        return THROW(TypeError, "expected (mode: String)");
    }
}

Handle<Value> Image::SetMasked(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (Image::HasInstance(args[0]) && args[1]->IsNumber()) {
        Pix *mask = Image::Pixels(args[0]->ToObject());
        int value = args[1]->Int32Value();
        if (pixSetMasked(obj->pix_, mask, value) == 1) {
            return THROW(TypeError, "error while applying mask");
        }
        return args.This();
    } else {
        return THROW(TypeError, "expected (image: Image, value: Int32)");
    }
}

Handle<Value> Image::ApplyCurve(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsArray() &&
            args[0]->ToObject()->Get(String::New("length"))->Uint32Value() == 256) {
        NUMA *numa = numaCreate(256);
        for (int i = 0; i < 256; i++) {
            numaAddNumber(numa, (l_float32)args[0]->ToObject()->Get(i)->ToInt32()->Value());
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
        return THROW(TypeError, "expected (array: Int32[256])");
    }
}

Handle<Value> Image::RankFilter(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsNumber() && args[1]->IsNumber() && args[2]->IsNumber()) {
        int width = static_cast<int>(ceil(args[0]->NumberValue()));
        int height = static_cast<int>(ceil(args[1]->NumberValue()));
        float rank = static_cast<float>(args[2]->NumberValue());
        PIX *pixd = pixRankFilter(obj->pix_, width, height, rank);
        if (pixd == NULL) {
            return THROW(TypeError, "error while applying rank filter");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (width: Number, height: Number, rank: Number)");
    }
}

Handle<Value> Image::OctreeColorQuant(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsInt32()) {
        int colors = args[0]->Int32Value();
        PIX *pixd = pixOctreeColorQuant(obj->pix_, colors, 0);
        if (pixd == NULL) {
            return THROW(TypeError, "error while quantizating");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (colors: Int32)");
    }
}

Handle<Value> Image::MedianCutQuant(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsInt32()) {
        int colors = args[0]->Int32Value();
        PIX *pixd = pixMedianCutQuantGeneral(obj->pix_, 0, 0, colors, 0, 1, 1);
        if (pixd == NULL) {
            return THROW(TypeError, "error while quantizating");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (colors: Int32)");
    }
}

Handle<Value> Image::Threshold(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 0 || args[0]->IsInt32()) {
        int value = args.Length() == 0 ? 128 : args[0]->Int32Value();
        PIX *pixd = pixConvertTo1(obj->pix_, value);
        if (pixd == NULL) {
            return THROW(TypeError, "error while thresholding");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (value: Int32)");
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
    } else if (args.Length() == 1 && args[0]->IsString()) {
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
    } else if (args.Length() == 3 && args[0]->IsNumber()
               && args[1]->IsNumber() && args[2]->IsNumber()) {
        float rwt = static_cast<float>(args[0]->NumberValue());
        float gwt = static_cast<float>(args[1]->NumberValue());
        float bwt = static_cast<float>(args[2]->NumberValue());
        PIX *grayPix = pixConvertRGBToGray(
                    obj->pix_, rwt, gwt, bwt);
        if (grayPix != NULL) {
            return scope.Close(Image::New(grayPix));
        } else {
            return THROW(Error, "error while computing grayscale image");
        }
    } else {
        return THROW(TypeError, "expected (rwt: Number, gwt: Number, bwt: Number) "
                     "or (type: String) or no arguments at all");
    }
}

Handle<Value> Image::ToColor(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    return scope.Close(Image::New(pixConvertTo32(obj->pix_)));
}

Handle<Value> Image::ToHSV(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    PIX *pixd = pixConvertRGBToHSV(NULL, obj->pix_);
    if (pixd != NULL) {
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(Error, "error while converting to HSV");
    }
}

Handle<Value> Image::ToRGB(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    PIX *pixd = pixConvertHSVToRGB(NULL, obj->pix_);
    if (pixd != NULL) {
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(Error, "error while converting to RGB");
    }
}

Handle<Value> Image::Erode(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsNumber() && args[1]->IsNumber()) {
        int width = static_cast<int>(ceil(args[0]->NumberValue()));
        int height = static_cast<int>(ceil(args[1]->NumberValue()));
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
        return THROW(TypeError, "expected (width: Number, height: Number)");
    }
}

Handle<Value> Image::Dilate(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsNumber() && args[1]->IsNumber()) {
        int width = static_cast<int>(ceil(args[0]->NumberValue()));
        int height = static_cast<int>(ceil(args[1]->NumberValue()));
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
        return THROW(TypeError, "expected (width: Number, height: Number)");
    }
}

Handle<Value> Image::Open(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsNumber() && args[1]->IsNumber()) {
        int width = static_cast<int>(ceil(args[0]->NumberValue()));
        int height = static_cast<int>(ceil(args[1]->NumberValue()));
        PIX *pixd = 0;
        if (obj->pix_->d == 1) {
            pixd = pixOpenBrick(NULL, obj->pix_, width, height);
        } else {
            pixd = pixOpenGray(obj->pix_, width, height);
        }
        if (pixd == NULL) {
            return THROW(TypeError, "error while opening");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (width: Number, height: Number)");
    }
}

Handle<Value> Image::Close(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsNumber() && args[1]->IsNumber()) {
        int width = static_cast<int>(ceil(args[0]->NumberValue()));
        int height = static_cast<int>(ceil(args[1]->NumberValue()));
        PIX *pixd = 0;
        if (obj->pix_->d == 1) {
            pixd = pixCloseBrick(NULL, obj->pix_, width, height);
        } else {
            pixd = pixCloseGray(obj->pix_, width, height);
        }
        if (pixd == NULL) {
            return THROW(TypeError, "error while closing");
        }
        return scope.Close(Image::New(pixd));
    } else {
        return THROW(TypeError, "expected (width: Number, height: Number)");
    }
}

Handle<Value> Image::Thin(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsString() && args[1]->IsInt32() && args[2]->IsInt32()) {
        int typeInt = 0;
        String::AsciiValue type(args[0]->ToString());
        if (strcmp("fg", *type) == 0) {
            typeInt = L_THIN_FG;
        } else if (strcmp("bg", *type) == 0) {
            typeInt = L_THIN_BG;
        }
        int connectivity = args[1]->Int32Value();
        int maxIters = args[2]->Int32Value();
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
        return THROW(TypeError, "expected (type: String, connectivity: Int32, maxIters: Int32)");
    }
}

Handle<Value> Image::MaxDynamicRange(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsString()) {
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
        return THROW(TypeError, "expected (type: String)");
    }
}

Handle<Value> Image::OtsuAdaptiveThreshold(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsInt32() && args[1]->IsInt32()
            && args[2]->IsInt32() && args[3]->IsInt32()
            && args[4]->IsNumber()) {
        int32_t sx = args[0]->Int32Value();
        int32_t sy = args[1]->Int32Value();
        int32_t smoothx = args[2]->Int32Value();
        int32_t smoothy = args[3]->Int32Value();
        float scorefact = static_cast<float>(args[4]->NumberValue());
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
        return THROW(TypeError, "expected (sx: Int32, sy: Int32, "
                     "smoothx: Int32, smoothy: Int32, scoreFact: Number)");
    }
}

Handle<Value> Image::LineSegments(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsBoolean()) {
        if (obj->pix_->d != 8) {
            return THROW(TypeError, "Not a 8bpp Image");
        }
        int accuracy = args[0]->Int32Value();
        int maxLineSegments = args[1]->Int32Value();
        bool useWMS = args[2]->BooleanValue();
        LSWMS lswms(cv::Size(obj->pix_->w, obj->pix_->h), accuracy,
                    maxLineSegments, useWMS, false);
        cv::Mat img(pix8ToMat(obj->pix_));
        std::vector<LSWMS::LSEG> lSegs;
        std::vector<double> errors;
        lswms.run(img, lSegs, errors);
        Local<Array> results = Array::New();
        for(size_t i = 0; i < lSegs.size(); i++) {
            Handle<Object> p1 = Object::New();
            p1->Set(String::NewSymbol("x"), Int32::New(lSegs[i][0].x));
            p1->Set(String::NewSymbol("y"), Int32::New(lSegs[i][0].y));
            Handle<Object> p2 = Object::New();
            p2->Set(String::NewSymbol("x"), Int32::New(lSegs[i][1].x));
            p2->Set(String::NewSymbol("y"), Int32::New(lSegs[i][1].y));
            Handle<Object> result = Object::New();
            result->Set(String::NewSymbol("p1"), p1);
            result->Set(String::NewSymbol("p2"), p2);
            result->Set(String::NewSymbol("error"), Number::New(errors[i]));
            results->Set(i, result);
        }
        return scope.Close(results);
    } else {
        return THROW(TypeError, "expected (accuracy: Int32, maxLineSegments: Int32, useWeightedMeanShift: Boolean)");
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
    if (args[0]->IsInt32()) {
        int connectivity = args[0]->Int32Value();
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
        return THROW(TypeError, "expected (connectivity: Int32)");
    }
}

Handle<Value> Image::DistanceFunction(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsInt32()) {
        int connectivity = args[0]->Int32Value();
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

        return THROW(TypeError, "expected (connectivity: Int32)");
    }
}

Handle<Value> Image::ClearBox(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    Box* box = toBox(args, 0);
    if (box) {
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
        return THROW(TypeError, "expected (box: Box) signature");
    }
}

Handle<Value> Image::FillBox(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    int boxEnd;
    BOX *box = toBox(args, 0, &boxEnd);
    if (box) {
        int error = 0;
        if (args[boxEnd + 1]->IsInt32() && !args[boxEnd + 2]->IsInt32()) {
            if (obj->pix_->d != 8 && obj->pix_->d != 1) {
                boxDestroy(&box);
                return THROW(TypeError, "Not a 8bpp or 1bpp Image");
            }
            int value = args[boxEnd + 1]->Int32Value();
            error = pixSetInRectArbitrary(obj->pix_, box, value);
        }
        else if (args[boxEnd + 1]->IsInt32() && args[boxEnd + 2]->IsInt32()
                 && args[boxEnd + 3]->IsInt32()) {
            if (obj->pix_->d < 32) {
                boxDestroy(&box);
                return THROW(TypeError, "Not a 32bpp Image");
            }
            uint8_t r = args[boxEnd + 1]->Int32Value();
            uint8_t g = args[boxEnd + 2]->Int32Value();
            uint8_t b = args[boxEnd + 3]->Int32Value();
            l_uint32 pixel;
            composeRGBPixel(r, g, b, &pixel);
            if (args[boxEnd + 4]->IsNumber()) {
                float fract = static_cast<float>(args[boxEnd + 4]->NumberValue());
                error = pixBlendInRect(obj->pix_, box, pixel, fract);
            }
            else {
                error = pixSetInRectArbitrary(obj->pix_, box, pixel);
            }
        }
        else {
            boxDestroy(&box);
            return THROW(TypeError, "expected (box: Box, value: Int32) or "
                         "(box: Box, r: Int32, g: Int32, b: Int32, [frac: Number])");
        }
        boxDestroy(&box);
        if (error) {
            return THROW(TypeError, "error while drawing box");
        }
        return args.This();
    }
    else {
        return THROW(TypeError, "expected (box: Box, value: Int32) or "
                     "(box: Box, r: Int32, g: Int32, b: Int32, [frac: Number])");
    }
}

Handle<Value> Image::DrawBox(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    int boxEnd;
    BOX *box = toBox(args, 0, &boxEnd);
    if (box && args[boxEnd + 1]->IsInt32()) {
        int borderWidth = args[boxEnd + 1]->ToInt32()->Value();
        int error = 0;
        if (args[boxEnd + 2]->IsString()) {
            int op  = toOp(args[boxEnd + 2]);
            if (op == -1) {
                boxDestroy(&box);
                return THROW(TypeError, "invalid op");
            }
            error = pixRenderBox(obj->pix_, box, borderWidth, op);
        } else if (args[boxEnd + 2]->IsInt32() && args[boxEnd + 3]->IsInt32()
                   && args[boxEnd + 4]->IsInt32()) {
            if (obj->pix_->d < 32) {
                boxDestroy(&box);
                return THROW(TypeError, "Not a 32bpp Image");
            }
            uint8_t r = args[boxEnd + 2]->Int32Value();
            uint8_t g = args[boxEnd + 3]->Int32Value();
            uint8_t b = args[boxEnd + 4]->Int32Value();
            if (args[boxEnd + 5]->IsNumber()) {
                float fract = static_cast<float>(args[boxEnd + 5]->NumberValue());
                error = pixRenderBoxBlend(obj->pix_, box, borderWidth, r, g, b, fract);
            } else {
                error = pixRenderBoxArb(obj->pix_, box, borderWidth, r, g, b);
            }
        } else {
            boxDestroy(&box);
            return THROW(TypeError, "expected (box: Box, borderWidth: Int32, "
                         "op: String) or (box: Box, borderWidth: Int32, r: Int32, "
                         "g: Int32, b: Int32, [frac: Number])");
        }
        boxDestroy(&box);
        if (error) {
            return THROW(TypeError, "error while drawing box");
        }
        return args.This();
    } else {
        return THROW(TypeError, "expected (box: Box, borderWidth: Int32, "
                     "op: String) or (box: Box, borderWidth: Int32, r: Int32, "
                     "g: Int32, b: Int32, [frac: Number])");
    }
}

Handle<Value> Image::DrawLine(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsObject() && args[1]->IsObject() && args[2]->IsInt32()) {
        Handle<Object> p1 = args[0]->ToObject();
        Handle<Object> p2 = args[1]->ToObject();
        l_int32 x1 = round(p1->Get(String::NewSymbol("x"))->ToNumber()->Value());
        l_int32 y1 = round(p1->Get(String::NewSymbol("y"))->ToNumber()->Value());
        l_int32 x2 = round(p2->Get(String::NewSymbol("x"))->ToNumber()->Value());
        l_int32 y2 = round(p2->Get(String::NewSymbol("y"))->ToNumber()->Value());
        l_int32 width = args[2]->Int32Value();
        l_int32 error;
        if (args[3]->IsString()) {
            int op  = toOp(args[3]);
            if (op == -1) {
                return THROW(TypeError, "invalid op");
            }
            error = pixRenderLine(obj->pix_, x1, y1, x2, y2, width, op);
        } else if (args[3]->IsInt32() && args[4]->IsInt32() && args[5]->IsInt32()) {
            if (obj->pix_->d < 32) {
                return THROW(TypeError, "Not a 32bpp Image");
            }
            uint8_t r = args[3]->Int32Value();
            uint8_t g = args[4]->Int32Value();
            uint8_t b = args[5]->Int32Value();
            if (args[6]->IsNumber()) {
                float fract = static_cast<float>(args[6]->NumberValue());
                error = pixRenderLineBlend(obj->pix_, x1, y1, x2, y2, width, r, g, b, fract);
            } else {
                error = pixRenderLineArb(obj->pix_, x1, y1, x2, y2, width, r, g, b);
            }
        } else {
             return THROW(TypeError, "expected (p1: Point, p2: Point, "
                          "width: Int32, op: String) or (p1: Point, p2: Point, "
                          "width: Int32, r: Int32, g: Int32, b: Int32, "
                          "[frac: Number])");
        }
        if (error) {
            return THROW(TypeError, "error while drawing line");
        }
        return args.This();
    } else {
        return THROW(TypeError, "expected (p1: Point, p2: Point, "
                     "width: Int32, op: String) or (p1: Point, p2: Point, "
                     "width: Int32, r: Int32, g: Int32, b: Int32, "
                     "[frac: Number])");
    }
}

Handle<Value> Image::DrawImage(const Arguments &args)
{
    HandleScope scope;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    int boxEnd;
    BOX *box = toBox(args, 1, &boxEnd);
    if (Image::HasInstance(args[0]) && box) {
        PIX *otherPix = Image::Pixels(args[0]->ToObject());
        int error = pixRasterop(obj->pix_, box->x, box->y, box->w, box->h,
                                PIX_SRC, otherPix, 0, 0);
        boxDestroy(&box);
        if (error) {
            return THROW(TypeError, "error while drawing image");
        }
        return args.This();
    } else {
        return THROW(TypeError, "expected (image: Image, box: Box)");
    }
}

Handle<Value> Image::ToBuffer(const Arguments &args)
{
    HandleScope scope;
    const int FORMAT_RAW = 0;
    const int FORMAT_PNG = 1;
    const int FORMAT_JPG = 2;
    int formatInt = FORMAT_RAW;
    jpge::params params;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() >= 1 && args[0]->IsString()) {
        String::AsciiValue format(args[0]->ToString());
        if (strcmp("raw", *format) == 0) {
            formatInt = FORMAT_RAW;
        } else if (strcmp("png", *format) == 0) {
            formatInt = FORMAT_PNG;
        } else if (strcmp("jpg", *format) == 0) {
            formatInt = FORMAT_JPG;
            if (args[1]->IsNumber()) {
                params.m_quality = args[1]->Int32Value();
            }
        } else {
            std::stringstream msg;
            msg << "invalid format '" << *format << "'";
            return THROW(Error, msg.str().c_str());
        }
    }
    std::vector<unsigned char> imgData;
    std::vector<unsigned char> pngData;
    char *jpgData;
    int jpgDataSize;
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
        if (formatInt == FORMAT_PNG) {
            lodepng::State state;
            state.info_png.color.colortype = LCT_RGB;
            state.info_png.color.bitdepth = 8;
            state.info_raw.colortype = LCT_RGB;
            error = lodepng::encode(pngData, imgData, obj->pix_->w, obj->pix_->h, state);
        } else if (formatInt == FORMAT_JPG) {
            jpgDataSize = obj->pix_->w * obj->pix_->h * 3 + 1024;
            jpgData = (char*)malloc(jpgDataSize);
            if (!jpge::compress_image_to_jpeg_file_in_memory(
                        jpgData, jpgDataSize, obj->pix_->w, obj->pix_->h,
                        3, &imgData[0], params)) {
                error = 1;
            }
        }
    } else if (obj->pix_->d <= 8) {
        PIX *pix8 = pixConvertTo8(obj->pix_, obj->pix_->colormap ? 1 : 0);
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
        if (formatInt == FORMAT_PNG) {
            lodepng::State state;
            if (obj->pix_->colormap) {
                state.info_png.color.colortype = LCT_PALETTE;
                state.info_png.color.bitdepth = obj->pix_->d;
                if (obj->pix_->d == 8)
                    state.encoder.auto_convert = LAC_NO;
                state.info_png.color.palettesize = pixcmapGetCount(obj->pix_->colormap);
                state.info_png.color.palette = new unsigned char[1024];
                state.info_raw.palettesize = pixcmapGetCount(obj->pix_->colormap);
                state.info_raw.palette = new unsigned char[1024];
                for (size_t i = 0; i < state.info_png.color.palettesize * 4; i += 4) {
                    int32_t r, g, b;
                    pixcmapGetColor(obj->pix_->colormap, static_cast<l_int32>(i / 4), &r, &g, &b);
                    state.info_png.color.palette[i+0] = r;
                    state.info_png.color.palette[i+1] = g;
                    state.info_png.color.palette[i+2] = b;
                    state.info_png.color.palette[i+3] = 255;
                    state.info_raw.palette[i+0] = r;
                    state.info_raw.palette[i+1] = g;
                    state.info_raw.palette[i+2] = b;
                    state.info_raw.palette[i+3] = 255;
                }
                state.info_raw.colortype = LCT_PALETTE;
            } else {
                state.info_png.color.colortype = LCT_GREY;
                state.info_png.color.bitdepth = obj->pix_->d;
                state.info_raw.colortype = LCT_GREY;
            }
            error = lodepng::encode(pngData, imgData, pix8->w, pix8->h, state);
            pixDestroy(&pix8);
        } else if (formatInt == FORMAT_JPG) {
            if (obj->pix_->colormap) {
                PIX* rgbPix = pixConvertTo32(obj->pix_);
                // Image is RGB, so create a 3 byte per pixel image.
                imgData.clear();
                uint32_t *line;
                imgData.reserve(rgbPix->w * rgbPix->h * 3);
                line = rgbPix->data;
                for (uint32_t y = 0; y < rgbPix->h; ++y) {
                    for (uint32_t x = 0; x < rgbPix->w; ++x) {
                        int32_t rval, gval, bval;
                        extractRGBValues(line[x], &rval, &gval, &bval);
                        imgData.push_back(rval);
                        imgData.push_back(gval);
                        imgData.push_back(bval);
                    }
                    line += rgbPix->wpl;
                }
                pixDestroy(&rgbPix);
                // To JPG
                jpgDataSize = obj->pix_->w * obj->pix_->h * 3 + 1024;
                jpgData = (char*)malloc(jpgDataSize);
                if (!jpge::compress_image_to_jpeg_file_in_memory(
                            jpgData, jpgDataSize, obj->pix_->w, obj->pix_->h,
                            3, &imgData[0], params)) {
                    error = 1;
                }
            } else {
                // To JPG
                jpgDataSize = obj->pix_->w * obj->pix_->h + 1024;
                jpgData = (char*)malloc(jpgDataSize);
                if (!jpge::compress_image_to_jpeg_file_in_memory(
                            jpgData, jpgDataSize, obj->pix_->w, obj->pix_->h,
                            1, &imgData[0], params)) {
                    error = 1;
                }
            }
        }
    } else {
        return THROW(Error, "invalid PIX depth");
    }
    if (error) {
        std::stringstream msg;
        msg << "error while encoding '" << lodepng_error_text(error) << "'";
        return THROW(Error, msg.str().c_str());
    }
    if (formatInt == FORMAT_PNG) {
        return scope.Close(Buffer::New(reinterpret_cast<char *>(&pngData[0]), pngData.size())->handle_);
    } else if (formatInt == FORMAT_JPG) {
        Handle<Value> buffer = Buffer::New(jpgData, jpgDataSize)->handle_;
        free(jpgData);
        return scope.Close(buffer);
    } else {
        return scope.Close(Buffer::New(reinterpret_cast<char *>(&imgData[0]), imgData.size())->handle_);
    }
}

Image::Image(Pix *pix)
    : pix_(pix)
{
    if (pix_) {
        V8::AdjustAmountOfExternalAllocatedMemory(size());
    }
}

Image::~Image()
{
    if (pix_) {
        V8::AdjustAmountOfExternalAllocatedMemory(-size());
        pixDestroy(&pix_);
    }
}

int Image::size() const
{
    return pix_->h * pix_->wpl * sizeof(uint32_t);
}

}
