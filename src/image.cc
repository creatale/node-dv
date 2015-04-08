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
    return NanHasInstance(constructor_template, val->ToObject());
}

Pix *Image::Pixels(Handle<Object> obj)
{
    return ObjectWrap::Unwrap<Image>(obj)->pix_;
}

void Image::Init(Handle<Object> target)
{   
    Local<FunctionTemplate> local_constructor_template = NanNew<FunctionTemplate>(New);
    local_constructor_template->SetClassName(NanNew("Image"));
    local_constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    Local<ObjectTemplate> proto = local_constructor_template->PrototypeTemplate();
    proto->SetAccessor(NanNew("width"), GetWidth);
    proto->SetAccessor(NanNew("height"), GetHeight);
    proto->SetAccessor(NanNew("depth"), GetDepth);
    proto->Set(NanNew("invert"),
               NanNew<FunctionTemplate>(Invert)->GetFunction());
    proto->Set(NanNew("or"),
               NanNew<FunctionTemplate>(Or)->GetFunction());
    proto->Set(NanNew("and"),
               NanNew<FunctionTemplate>(And)->GetFunction());
    proto->Set(NanNew("xor"),
               NanNew<FunctionTemplate>(Xor)->GetFunction());
    proto->Set(NanNew("add"),
               NanNew<FunctionTemplate>(Add)->GetFunction());
    proto->Set(NanNew("subtract"),
               NanNew<FunctionTemplate>(Subtract)->GetFunction());
    proto->Set(NanNew("convolve"),
               NanNew<FunctionTemplate>(Convolve)->GetFunction());
    proto->Set(NanNew("unsharpMasking"), //TODO: remove (deprecated).
               NanNew<FunctionTemplate>(Unsharp)->GetFunction());
    proto->Set(NanNew("unsharp"),
               NanNew<FunctionTemplate>(Unsharp)->GetFunction());
    proto->Set(NanNew("rotate"),
               NanNew<FunctionTemplate>(Rotate)->GetFunction());
    proto->Set(NanNew("scale"),
               NanNew<FunctionTemplate>(Scale)->GetFunction());
    proto->Set(NanNew("crop"),
               NanNew<FunctionTemplate>(Crop)->GetFunction());
    proto->Set(NanNew("inRange"),
               NanNew<FunctionTemplate>(InRange)->GetFunction());
    proto->Set(NanNew("histogram"),
               NanNew<FunctionTemplate>(Histogram)->GetFunction());
    proto->Set(NanNew("projection"),
               NanNew<FunctionTemplate>(Projection)->GetFunction());
    proto->Set(NanNew("setMasked"),
               NanNew<FunctionTemplate>(SetMasked)->GetFunction());
    proto->Set(NanNew("applyCurve"),
               NanNew<FunctionTemplate>(ApplyCurve)->GetFunction());
    proto->Set(NanNew("rankFilter"),
               NanNew<FunctionTemplate>(RankFilter)->GetFunction());
    proto->Set(NanNew("octreeColorQuant"),
               NanNew<FunctionTemplate>(OctreeColorQuant)->GetFunction());
    proto->Set(NanNew("medianCutQuant"),
               NanNew<FunctionTemplate>(MedianCutQuant)->GetFunction());
    proto->Set(NanNew("threshold"),
               NanNew<FunctionTemplate>(Threshold)->GetFunction());
    proto->Set(NanNew("toGray"),
               NanNew<FunctionTemplate>(ToGray)->GetFunction());
    proto->Set(NanNew("toColor"),
               NanNew<FunctionTemplate>(ToColor)->GetFunction());
    proto->Set(NanNew("toHSV"),
               NanNew<FunctionTemplate>(ToHSV)->GetFunction());
    proto->Set(NanNew("toRGB"),
               NanNew<FunctionTemplate>(ToRGB)->GetFunction());
    proto->Set(NanNew("erode"),
               NanNew<FunctionTemplate>(Erode)->GetFunction());
    proto->Set(NanNew("dilate"),
               NanNew<FunctionTemplate>(Dilate)->GetFunction());
    proto->Set(NanNew("open"),
               NanNew<FunctionTemplate>(Open)->GetFunction());
    proto->Set(NanNew("close"),
               NanNew<FunctionTemplate>(Close)->GetFunction());
    proto->Set(NanNew("thin"),
               NanNew<FunctionTemplate>(Thin)->GetFunction());
    proto->Set(NanNew("maxDynamicRange"),
               NanNew<FunctionTemplate>(MaxDynamicRange)->GetFunction());
    proto->Set(NanNew("otsuAdaptiveThreshold"),
               NanNew<FunctionTemplate>(OtsuAdaptiveThreshold)->GetFunction());
    proto->Set(NanNew("lineSegments"),
               NanNew<FunctionTemplate>(LineSegments)->GetFunction());
    proto->Set(NanNew("findSkew"),
               NanNew<FunctionTemplate>(FindSkew)->GetFunction());
    proto->Set(NanNew("connectedComponents"),
               NanNew<FunctionTemplate>(ConnectedComponents)->GetFunction());
    proto->Set(NanNew("distanceFunction"),
               NanNew<FunctionTemplate>(DistanceFunction)->GetFunction());
    proto->Set(NanNew("clearBox"), //TODO: remove (deprecated).
               NanNew<FunctionTemplate>(ClearBox)->GetFunction());
    proto->Set(NanNew("fillBox"),
               NanNew<FunctionTemplate>(FillBox)->GetFunction());
    proto->Set(NanNew("drawBox"),
               NanNew<FunctionTemplate>(DrawBox)->GetFunction());
    proto->Set(NanNew("drawImage"),
               NanNew<FunctionTemplate>(DrawImage)->GetFunction());
    proto->Set(NanNew("drawLine"),
               NanNew<FunctionTemplate>(DrawLine)->GetFunction());
    proto->Set(NanNew("toBuffer"),
               NanNew<FunctionTemplate>(ToBuffer)->GetFunction());
    
    NanAssignPersistent(constructor_template, local_constructor_template);

    target->Set(NanNew("Image"), local_constructor_template->GetFunction());
}

Handle<Value> Image::New(Pix *pix)
{
    NanEscapableScope();
    Local<Object> instance = NanNew(constructor_template)->GetFunction()->NewInstance();
    Image *obj = ObjectWrap::Unwrap<Image>(instance);
    obj->pix_ = pix;
    if (obj->pix_) {
        NanAdjustExternalMemory(obj->size());
    }
    return NanEscapeScope(instance);
}

NAN_METHOD(Image::New)
{
    NanScope();
    Pix *pix;
    if (args.Length() == 0) {
        pix = 0;
    } else if (args.Length() == 1 &&  Image::HasInstance(args[0])) {
        pix = pixCopy(NULL, Image::Pixels(args[0]->ToObject()));
    } else if (args.Length() == 2 && Buffer::HasInstance(args[1])) {
        String::Utf8Value format(args[0]->ToString());
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
                return NanThrowError(msg.str().c_str());
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
                return NanThrowError("error while decoding jpg");
            }
            pix = pixFromSource(out, width, height, 32, comps == 1 ? 8 : 32);
            free(out);
        } else {
            std::stringstream msg;
            msg << "invalid bufffer format '" << *format << "'";
            return NanThrowError(msg.str().c_str());
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
        String::Utf8Value format(args[0]->ToString());
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
            return NanThrowError(msg.str().c_str());
        }
        size_t expectedLength = width * height * depth;
        if (expectedLength != length << 3) {
            return NanThrowError("invalid Buffer length");
        }
        pix = pixFromSource(reinterpret_cast<uint8_t*>(Buffer::Data(buffer)), width, height, depth, 32);
    } else {
        return NanThrowTypeError("expected (image: Image) or (image1: Image, "
                     "image2: Image, image3: Image) or (format: String, "
                     "image: Buffer, [width: Int32, height: Int32]) or no arguments at all");
    }
    Image* obj = new Image(pix);
    obj->Wrap(args.This());
    NanReturnThis();
}

NAN_GETTER(Image::GetWidth)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    NanReturnValue(NanNew<Number>(obj->pix_->w));
}

NAN_GETTER(Image::GetHeight)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    NanReturnValue(NanNew<Number>(obj->pix_->h));
}

NAN_GETTER(Image::GetDepth)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    NanReturnValue(NanNew<Number>(obj->pix_->d));
}

NAN_METHOD(Image::Invert)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    Pix *pixd = pixInvert(NULL, obj->pix_);
    if (pixd == NULL) {
        return NanThrowTypeError("error while applying INVERT");
    }
    NanReturnValue(Image::New(pixd));
}

NAN_METHOD(Image::Or)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (Image::HasInstance(args[0])) {
        Pix *otherPix = Image::Pixels(args[0]->ToObject());
        Pix *pixd = pixOr(NULL, obj->pix_, otherPix);
        if (pixd == NULL) {
            return NanThrowTypeError("error while applying OR");
        }
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowTypeError("expected (image: Image)");
    }
}

NAN_METHOD(Image::And)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (Image::HasInstance(args[0])) {
        Pix *otherPix = Image::Pixels(args[0]->ToObject());
        Pix *pixd = pixAnd(NULL, obj->pix_, otherPix);
        if (pixd == NULL) {
            return NanThrowTypeError("error while applying AND");
        }
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowTypeError("expected (image: Image)");
    }
}

NAN_METHOD(Image::Xor)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (Image::HasInstance(args[0])) {
        Pix *otherPix = Image::Pixels(args[0]->ToObject());
        Pix *pixd = pixXor(NULL, obj->pix_, otherPix);
        if (pixd == NULL) {
            return NanThrowTypeError("error while applying XOR");
        }
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowTypeError("expected (image: Image)");
    }
}

NAN_METHOD(Image::Add)
{
    NanScope();
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
            return NanThrowTypeError("error while applying ADD");
        }
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowTypeError("expected (image: Image)");
    }
}

NAN_METHOD(Image::Subtract)
{
    NanScope();
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
            return NanThrowTypeError("error while applying SUBTRACT");
        }
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowTypeError("expected (image: Image)");
    }
}

NAN_METHOD(Image::Convolve)
{
    NanScope();
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
            return NanThrowTypeError("error while applying convolve");
        }
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowTypeError("expected (width: Number, height: Number)");
    }
}

NAN_METHOD(Image::Unsharp)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsNumber() && args[1]->IsNumber()) {
        int halfWidth = static_cast<int>(ceil(args[0]->NumberValue()));
        float fract = static_cast<float>(args[1]->NumberValue());
        Pix *pixd = pixUnsharpMasking(obj->pix_, halfWidth, fract);
        if (pixd == NULL) {
            return NanThrowTypeError("error while applying unsharp");
        }
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowTypeError("expected (halfWidth: Number, fract: Number)");
    }
}

NAN_METHOD(Image::Rotate)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsNumber()) {
        const float deg2rad = 3.1415926535f / 180.0f;
        float angle = static_cast<float>(args[0]->NumberValue());
        Pix *pixd = pixRotate(obj->pix_, deg2rad * angle,
                              L_ROTATE_AREA_MAP, L_BRING_IN_WHITE,
                              obj->pix_->w, obj->pix_->h);
        if (pixd == NULL) {
            return NanThrowTypeError("error while applying rotate");
        }
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowTypeError("expected (angle: Number)");
    }
}

NAN_METHOD(Image::Scale)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsNumber() && (args.Length() != 2 || args[1]->IsNumber())) {
        float scaleX = static_cast<float>(args[0]->NumberValue());
        float scaleY = static_cast<float>(args.Length() == 2 ? args[1]->NumberValue() : scaleX);
        Pix *pixd = pixScale(obj->pix_, scaleX, scaleY);
        if (pixd == NULL) {
            return NanThrowTypeError("error while scaling");
        }
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowTypeError("expected (scaleX: Number, [scaleY: Number])");
    }
}

NAN_METHOD(Image::Crop)
{
    NanScope();
    Box* box = toBox(args, 0);
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (box) {
        PIX *pixd = pixClipRectangle(obj->pix_, box, 0);
        boxDestroy(&box);
        if (pixd == NULL) {
            return NanThrowTypeError("error while cropping");
        }
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowTypeError("expected (box: Box)");
    }
}

NAN_METHOD(Image::InRange)
{
    NanScope();
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
            return NanThrowTypeError("error while computing in range mask");
        }
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowTypeError("expected ("
                     "lower1: Number, lower2: Number, lower3: Number, "
                     "upper1: Number, upper2: Number, upper3: Number)");
    }
}

NAN_METHOD(Image::Histogram)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    PIX *mask = NULL;
    if (args.Length() >= 1 && Image::HasInstance(args[0])) {
        mask = Image::Pixels(args[1]->ToObject());
    }
    NUMA *hist = pixGetGrayHistogramMasked(obj->pix_, mask, 0, 0, obj->pix_->h > 400 ? 2 : 1);
    if (!hist) {
        return NanThrowTypeError("error while computing histogram");
    }
    int len = numaGetCount(hist);
    l_float32 total = 0;
    for (int i = 0; i < len; i++) {
        total += hist->array[i];
    }
    Local<Array> result = NanNew<Array>(len);
    for (int i = 0; i < len; i++) {
        result->Set(i, NanNew<Number>(hist->array[i] / total));
    }
    numaDestroy(&hist);
    NanReturnValue(result);
}

NAN_METHOD(Image::Projection)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsString()) {
        String::Utf8Value mode(args[0]->ToString());
        ProjectionMode modeEnum;
        if (strcmp("horizontal", *mode) == 0) {
            modeEnum = Horizontal;
        }
        else if (strcmp("vertical", *mode) == 0) {
            modeEnum = Vertical;
        }
        else {
            return NanThrowError("expected mode to be 'horizontal' or 'vertical'");
        }
        if (obj->pix_->d != 8 && obj->pix_->d != 1) {
            return NanThrowError("expected 8bpp or 1bpp image");
        }
        std::vector<uint32_t> values;
        pixProjection(values, obj->pix_, modeEnum);
        Local<Array> result = NanNew<Array>(static_cast<int>(values.size()));
        for (int i = 0; i < values.size(); i++) {
            result->Set(i, NanNew<Number>(values[i]));
        }
        NanReturnValue(result);
    }
    else {
        return NanThrowTypeError("expected (mode: String)");
    }
}

NAN_METHOD(Image::SetMasked)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (Image::HasInstance(args[0]) && args[1]->IsNumber()) {
        Pix *mask = Image::Pixels(args[0]->ToObject());
        int value = args[1]->Int32Value();
        if (pixSetMasked(obj->pix_, mask, value) == 1) {
            return NanThrowTypeError("error while applying mask");
        }
        NanReturnThis();
    } else {
        return NanThrowTypeError("expected (image: Image, value: Int32)");
    }
}

NAN_METHOD(Image::ApplyCurve)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsArray() &&
            args[0]->ToObject()->Get(NanNew<String>("length"))->Uint32Value() == 256) {
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
            return NanThrowTypeError("error while applying value mapping");
        }
        NanReturnThis();
    } else {
        return NanThrowTypeError("expected (array: Int32[256])");
    }
}

NAN_METHOD(Image::RankFilter)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsNumber() && args[1]->IsNumber() && args[2]->IsNumber()) {
        int width = static_cast<int>(ceil(args[0]->NumberValue()));
        int height = static_cast<int>(ceil(args[1]->NumberValue()));
        float rank = static_cast<float>(args[2]->NumberValue());
        PIX *pixd = pixRankFilter(obj->pix_, width, height, rank);
        if (pixd == NULL) {
            return NanThrowTypeError("error while applying rank filter");
        }
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowTypeError("expected (width: Number, height: Number, rank: Number)");
    }
}

NAN_METHOD(Image::OctreeColorQuant)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsInt32()) {
        int colors = args[0]->Int32Value();
        PIX *pixd = pixOctreeColorQuant(obj->pix_, colors, 0);
        if (pixd == NULL) {
            return NanThrowTypeError("error while quantizating");
        }
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowTypeError("expected (colors: Int32)");
    }
}

NAN_METHOD(Image::MedianCutQuant)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsInt32()) {
        int colors = args[0]->Int32Value();
        PIX *pixd = pixMedianCutQuantGeneral(obj->pix_, 0, 0, colors, 0, 1, 1);
        if (pixd == NULL) {
            return NanThrowTypeError("error while quantizating");
        }
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowTypeError("expected (colors: Int32)");
    }
}

NAN_METHOD(Image::Threshold)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 0 || args[0]->IsInt32()) {
        int value = args.Length() == 0 ? 128 : args[0]->Int32Value();
        PIX *pixd = pixConvertTo1(obj->pix_, value);
        if (pixd == NULL) {
            return NanThrowTypeError("error while thresholding");
        }
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowTypeError("expected (value: Int32)");
    }
}

NAN_METHOD(Image::ToGray)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (obj->pix_->d == 8) {
        NanReturnValue(Image::New(pixClone(obj->pix_)));
    }
    if (args.Length() == 0) {
        PIX *grayPix = pixConvertTo8(obj->pix_, 0);
        if (grayPix != NULL) {
            NanReturnValue(Image::New(grayPix));
        } else {
            return NanThrowError("error while computing grayscale image");
        }
    } else if (args.Length() == 1 && args[0]->IsString()) {
        String::Utf8Value type(args[0]->ToString());
        int32_t typeInt;
        if (strcmp("min", *type) == 0) {
            typeInt = L_CHOOSE_MIN;
        } else if (strcmp("max", *type) == 0) {
            typeInt = L_CHOOSE_MAX;
        } else {
            return NanThrowError("expected type to be 'min' or 'max'");
        }
        PIX *grayPix = pixConvertRGBToGrayMinMax(
                    obj->pix_, typeInt);
        if (grayPix != NULL) {
            NanReturnValue(Image::New(grayPix));
        } else {
            return NanThrowError("error while computing grayscale image");
        }
    } else if (args.Length() == 3 && args[0]->IsNumber()
               && args[1]->IsNumber() && args[2]->IsNumber()) {
        float rwt = static_cast<float>(args[0]->NumberValue());
        float gwt = static_cast<float>(args[1]->NumberValue());
        float bwt = static_cast<float>(args[2]->NumberValue());
        PIX *grayPix = pixConvertRGBToGray(
                    obj->pix_, rwt, gwt, bwt);
        if (grayPix != NULL) {
            NanReturnValue(Image::New(grayPix));
        } else {
            return NanThrowError("error while computing grayscale image");
        }
    } else {
        return NanThrowTypeError("expected (rwt: Number, gwt: Number, bwt: Number) "
                     "or (type: String) or no arguments at all");
    }
}

NAN_METHOD(Image::ToColor)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    NanReturnValue(Image::New(pixConvertTo32(obj->pix_)));
}

NAN_METHOD(Image::ToHSV)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    PIX *pixd = pixConvertRGBToHSV(NULL, obj->pix_);
    if (pixd != NULL) {
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowError("error while converting to HSV");
    }
}

NAN_METHOD(Image::ToRGB)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    PIX *pixd = pixConvertHSVToRGB(NULL, obj->pix_);
    if (pixd != NULL) {
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowError("error while converting to RGB");
    }
}

NAN_METHOD(Image::Erode)
{
    NanScope();
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
            return NanThrowTypeError("error while eroding");
        }
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowTypeError("expected (width: Number, height: Number)");
    }
}

NAN_METHOD(Image::Dilate)
{
    NanScope();
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
            return NanThrowTypeError("error while dilating");
        }
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowTypeError("expected (width: Number, height: Number)");
    }
}

NAN_METHOD(Image::Open)
{
    NanScope();
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
            return NanThrowTypeError("error while opening");
        }
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowTypeError("expected (width: Number, height: Number)");
    }
}

NAN_METHOD(Image::Close)
{
    NanScope();
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
            return NanThrowTypeError("error while closing");
        }
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowTypeError("expected (width: Number, height: Number)");
    }
}

NAN_METHOD(Image::Thin)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsString() && args[1]->IsInt32() && args[2]->IsInt32()) {
        int typeInt = 0;
        String::Utf8Value type(args[0]->ToString());
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
            return NanThrowTypeError("error while thinning");
        }
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowTypeError("expected (type: String, connectivity: Int32, maxIters: Int32)");
    }
}

NAN_METHOD(Image::MaxDynamicRange)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsString()) {
        int typeInt = 0;
        String::Utf8Value type(args[0]->ToString());
        if (strcmp("linear", *type) == 0) {
            typeInt = L_SET_PIXELS;
        } else if (strcmp("log", *type) == 0) {
            typeInt = L_LOG_SCALE;
        }
        PIX *pixd = pixMaxDynamicRange(obj->pix_, typeInt);
        if (pixd == NULL) {
            return NanThrowTypeError("error while computing max. dynamic range");
        }
        NanReturnValue(Image::New(pixd));
    } else {
        return NanThrowTypeError("expected (type: String)");
    }
}

NAN_METHOD(Image::OtsuAdaptiveThreshold)
{
    NanScope();
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
            Local<Object> object = NanNew<Object>();
            object->Set(NanNew("thresholdValues"), Image::New(ppixth));
            object->Set(NanNew("image"), Image::New(ppixd));
            NanReturnValue(object);
        } else {
            return NanThrowError("error while computing threshold");
        }
    } else {
        return NanThrowTypeError("expected (sx: Int32, sy: Int32, "
                     "smoothx: Int32, smoothy: Int32, scoreFact: Number)");
    }
}

NAN_METHOD(Image::LineSegments)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsBoolean()) {
        if (obj->pix_->d != 8) {
            return NanThrowTypeError("Not a 8bpp Image");
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
        Local<Array> results = NanNew<Array>();
        for(size_t i = 0; i < lSegs.size(); i++) {
            Handle<Object> p1 = NanNew<Object>();
            p1->Set(NanNew("x"), NanNew<Int32>(lSegs[i][0].x));
            p1->Set(NanNew("y"), NanNew<Int32>(lSegs[i][0].y));
            Handle<Object> p2 = NanNew<Object>();
            p2->Set(NanNew("x"), NanNew<Int32>(lSegs[i][1].x));
            p2->Set(NanNew("y"), NanNew<Int32>(lSegs[i][1].y));
            Handle<Object> result = NanNew<Object>();
            result->Set(NanNew("p1"), p1);
            result->Set(NanNew("p2"), p2);
            result->Set(NanNew("error"), NanNew<Number>(errors[i]));
            results->Set(i, result);
        }
        NanReturnValue(results);
    } else {
        return NanThrowTypeError("expected (accuracy: Int32, maxLineSegments: Int32, useWeightedMeanShift: Boolean)");
    }
}

NAN_METHOD(Image::FindSkew)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    int32_t depth = pixGetDepth(obj->pix_);
    if (depth != 1) {
        return NanThrowTypeError("expected binarized image");
    }
    float angle;
    float conf;
    int error = pixFindSkew(obj->pix_, &angle, &conf);
    if (error == 0) {
        Local<Object> object = NanNew<Object>();
        object->Set(NanNew("angle"), NanNew<Number>(angle));
        object->Set(NanNew("confidence"), NanNew<Number>(conf));
        NanReturnValue(object);
    } else {
        return NanThrowError("angle measurment not valid");
    }
}

NAN_METHOD(Image::ConnectedComponents)
{
    NanScope();
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
            return NanThrowTypeError("error while computing connected components");
        }
        Local<Object> boxes = NanNew<Array>();
        for (int i = 0; i < boxa->n; ++i) {
            boxes->Set(i, createBox(boxa->box[i]));
        }
        boxaDestroy(&boxa);
        NanReturnValue(boxes);
    } else {
        return NanThrowTypeError("expected (connectivity: Int32)");
    }
}

NAN_METHOD(Image::DistanceFunction)
{
    NanScope();
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
            return NanThrowTypeError("error while computing distance function");
        }
        NanReturnValue(Image::New(pixDistance));
    } else {

        return NanThrowTypeError("expected (connectivity: Int32)");
    }
}

NAN_METHOD(Image::ClearBox)
{
    NanScope();
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
            return NanThrowTypeError("error while clearing box");
        }
        NanReturnThis();
    } else {
        return NanThrowTypeError("expected (box: Box) signature");
    }
}

NAN_METHOD(Image::FillBox)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    int boxEnd;
    BOX *box = toBox(args, 0, &boxEnd);
    if (box) {
        int error = 0;
        if (args[boxEnd + 1]->IsInt32() && !args[boxEnd + 2]->IsInt32()) {
            if (obj->pix_->d != 8 && obj->pix_->d != 1) {
                boxDestroy(&box);
                return NanThrowTypeError("Not a 8bpp or 1bpp Image");
            }
            int value = args[boxEnd + 1]->Int32Value();
            error = pixSetInRectArbitrary(obj->pix_, box, value);
        }
        else if (args[boxEnd + 1]->IsInt32() && args[boxEnd + 2]->IsInt32()
                 && args[boxEnd + 3]->IsInt32()) {
            if (obj->pix_->d < 32) {
                boxDestroy(&box);
                return NanThrowTypeError("Not a 32bpp Image");
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
            return NanThrowTypeError("expected (box: Box, value: Int32) or "
                         "(box: Box, r: Int32, g: Int32, b: Int32, [frac: Number])");
        }
        boxDestroy(&box);
        if (error) {
            return NanThrowTypeError("error while drawing box");
        }
        NanReturnThis();
    }
    else {
        return NanThrowTypeError("expected (box: Box, value: Int32) or "
                     "(box: Box, r: Int32, g: Int32, b: Int32, [frac: Number])");
    }
}

NAN_METHOD(Image::DrawBox)
{
    NanScope();
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
                return NanThrowTypeError("invalid op");
            }
            error = pixRenderBox(obj->pix_, box, borderWidth, op);
        } else if (args[boxEnd + 2]->IsInt32() && args[boxEnd + 3]->IsInt32()
                   && args[boxEnd + 4]->IsInt32()) {
            if (obj->pix_->d < 32) {
                boxDestroy(&box);
                return NanThrowTypeError("Not a 32bpp Image");
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
            return NanThrowTypeError("expected (box: Box, borderWidth: Int32, "
                         "op: String) or (box: Box, borderWidth: Int32, r: Int32, "
                         "g: Int32, b: Int32, [frac: Number])");
        }
        boxDestroy(&box);
        if (error) {
            return NanThrowTypeError("error while drawing box");
        }
        NanReturnThis();
    } else {
        return NanThrowTypeError("expected (box: Box, borderWidth: Int32, "
                     "op: String) or (box: Box, borderWidth: Int32, r: Int32, "
                     "g: Int32, b: Int32, [frac: Number])");
    }
}

NAN_METHOD(Image::DrawLine)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args[0]->IsObject() && args[1]->IsObject() && args[2]->IsInt32()) {
        Handle<Object> p1 = args[0]->ToObject();
        Handle<Object> p2 = args[1]->ToObject();
        l_int32 x1 = round(p1->Get(NanNew("x"))->ToNumber()->Value());
        l_int32 y1 = round(p1->Get(NanNew("y"))->ToNumber()->Value());
        l_int32 x2 = round(p2->Get(NanNew("x"))->ToNumber()->Value());
        l_int32 y2 = round(p2->Get(NanNew("y"))->ToNumber()->Value());
        l_int32 width = args[2]->Int32Value();
        l_int32 error;
        if (args[3]->IsString()) {
            int op  = toOp(args[3]);
            if (op == -1) {
                return NanThrowTypeError("invalid op");
            }
            error = pixRenderLine(obj->pix_, x1, y1, x2, y2, width, op);
        } else if (args[3]->IsInt32() && args[4]->IsInt32() && args[5]->IsInt32()) {
            if (obj->pix_->d < 32) {
                return NanThrowTypeError("Not a 32bpp Image");
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
             return NanThrowTypeError("expected (p1: Point, p2: Point, "
                          "width: Int32, op: String) or (p1: Point, p2: Point, "
                          "width: Int32, r: Int32, g: Int32, b: Int32, "
                          "[frac: Number])");
        }
        if (error) {
            return NanThrowTypeError("error while drawing line");
        }
        NanReturnThis();
    } else {
        return NanThrowTypeError("expected (p1: Point, p2: Point, "
                     "width: Int32, op: String) or (p1: Point, p2: Point, "
                     "width: Int32, r: Int32, g: Int32, b: Int32, "
                     "[frac: Number])");
    }
}

NAN_METHOD(Image::DrawImage)
{
    NanScope();
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    int boxEnd;
    BOX *box = toBox(args, 1, &boxEnd);
    if (Image::HasInstance(args[0]) && box) {
        PIX *otherPix = Image::Pixels(args[0]->ToObject());
        int error = pixRasterop(obj->pix_, box->x, box->y, box->w, box->h,
                                PIX_SRC, otherPix, 0, 0);
        boxDestroy(&box);
        if (error) {
            return NanThrowTypeError("error while drawing image");
        }
        NanReturnThis();
    } else {
        return NanThrowTypeError("expected (image: Image, box: Box)");
    }
}

NAN_METHOD(Image::ToBuffer)
{
    NanScope();
    const int FORMAT_RAW = 0;
    const int FORMAT_PNG = 1;
    const int FORMAT_JPG = 2;
    int formatInt = FORMAT_RAW;
    jpge::params params;
    Image *obj = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() >= 1 && args[0]->IsString()) {
        String::Utf8Value format(args[0]->ToString());
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
            return NanThrowError(msg.str().c_str());
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
        return NanThrowError("invalid PIX depth");
    }
    if (error) {
        std::stringstream msg;
        msg << "error while encoding '" << lodepng_error_text(error) << "'";
        return NanThrowError(msg.str().c_str());
    }
    if (formatInt == FORMAT_PNG) {
        NanReturnValue(NanNewBufferHandle(reinterpret_cast<char *>(&pngData[0]), pngData.size()));
    } else if (formatInt == FORMAT_JPG) {
        Handle<Value> buffer = NanNewBufferHandle(jpgData, jpgDataSize);
        free(jpgData);
        NanReturnValue(buffer);
    } else {
        NanReturnValue(NanNewBufferHandle(reinterpret_cast<char *>(&imgData[0]), imgData.size()));
    }
}

Image::Image(Pix *pix)
    : pix_(pix)
{
    if (pix_) {
        NanAdjustExternalMemory(size());
    }
}

Image::~Image()
{
    if (pix_) {
        NanAdjustExternalMemory(-size());
        pixDestroy(&pix_);
    }
}

int Image::size() const
{
    return pix_->h * pix_->wpl * sizeof(uint32_t);
}

}
