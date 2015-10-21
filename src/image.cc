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

namespace binding {

Nan::Persistent<FunctionTemplate> Image::constructor_template;

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
    return Nan::New(constructor_template)->HasInstance(val->ToObject());
}

Pix *Image::Pixels(Local<Object> obj)
{
    return Nan::ObjectWrap::Unwrap<Image>(obj)->pix_;
}

NAN_MODULE_INIT(Image::Init)
{  
    auto ctor = Nan::New<v8::FunctionTemplate>(New);
    auto ctorInst = ctor->InstanceTemplate();
    ctor->SetClassName(Nan::New("Image").ToLocalChecked());
    ctorInst->SetInternalFieldCount(1);
    
	Nan::SetAccessor(ctorInst, Nan::New("width").ToLocalChecked(), GetWidth);
	Nan::SetAccessor(ctorInst, Nan::New("height").ToLocalChecked(), GetHeight);
	Nan::SetAccessor(ctorInst, Nan::New("depth").ToLocalChecked(), GetDepth);
	
	Nan::SetPrototypeMethod(ctor, "invert", Invert);
	Nan::SetPrototypeMethod(ctor, "or", Or);
	Nan::SetPrototypeMethod(ctor, "and", And);
	Nan::SetPrototypeMethod(ctor, "xor", Xor);
	Nan::SetPrototypeMethod(ctor, "add", Add);
	Nan::SetPrototypeMethod(ctor, "subtract", Subtract);
    Nan::SetPrototypeMethod(ctor, "convolve", Convolve);
    Nan::SetPrototypeMethod(ctor, "unsharp", Unsharp);
    Nan::SetPrototypeMethod(ctor, "rotate", Rotate);
    Nan::SetPrototypeMethod(ctor, "scale", Scale);
    Nan::SetPrototypeMethod(ctor, "crop", Crop);
    Nan::SetPrototypeMethod(ctor, "inRange", InRange);
    Nan::SetPrototypeMethod(ctor, "histogram", Histogram);
    Nan::SetPrototypeMethod(ctor, "projection", Projection);
    Nan::SetPrototypeMethod(ctor, "setMasked", SetMasked);
    Nan::SetPrototypeMethod(ctor, "applyCurve", ApplyCurve);
    Nan::SetPrototypeMethod(ctor, "rankFilter", RankFilter);
    Nan::SetPrototypeMethod(ctor, "octreeColorQuant", OctreeColorQuant);
    Nan::SetPrototypeMethod(ctor, "medianCutQuant", MedianCutQuant);
    Nan::SetPrototypeMethod(ctor, "threshold", Threshold);
    Nan::SetPrototypeMethod(ctor, "toGray", ToGray);
    Nan::SetPrototypeMethod(ctor, "toColor", ToColor);
    Nan::SetPrototypeMethod(ctor, "toHSV", ToHSV);
    Nan::SetPrototypeMethod(ctor, "toRGB", ToRGB);
    Nan::SetPrototypeMethod(ctor, "erode", Erode);
    Nan::SetPrototypeMethod(ctor, "dilate", Dilate);
    Nan::SetPrototypeMethod(ctor, "open", Open);
    Nan::SetPrototypeMethod(ctor, "close", Close);
    Nan::SetPrototypeMethod(ctor, "thin", Thin);
    Nan::SetPrototypeMethod(ctor, "maxDynamicRange", MaxDynamicRange);
    Nan::SetPrototypeMethod(ctor, "otsuAdaptiveThreshold", OtsuAdaptiveThreshold);
    Nan::SetPrototypeMethod(ctor, "lineSegments", LineSegments);
    Nan::SetPrototypeMethod(ctor, "findSkew", FindSkew);
    Nan::SetPrototypeMethod(ctor, "connectedComponents", ConnectedComponents);
    Nan::SetPrototypeMethod(ctor, "distanceFunction", DistanceFunction);
    Nan::SetPrototypeMethod(ctor, "clearBox", ClearBox);
    Nan::SetPrototypeMethod(ctor, "fillBox", FillBox);
    Nan::SetPrototypeMethod(ctor, "drawBox", DrawBox);
    Nan::SetPrototypeMethod(ctor, "drawImage", DrawImage);
    Nan::SetPrototypeMethod(ctor, "drawLine", DrawLine);
	Nan::SetPrototypeMethod(ctor, "toBuffer", ToBuffer);
    
    constructor_template.Reset(ctor);

    Nan::Set(target, Nan::New("Image").ToLocalChecked(), Nan::GetFunction(ctor).ToLocalChecked());
}

Local<Object> Image::New(Pix *pix)
{
    Nan::EscapableHandleScope scope;
    Local<Object> instance = Nan::New(constructor_template)->GetFunction()->NewInstance();
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(instance);
    obj->pix_ = pix;
    if (obj->pix_) {
        Nan::AdjustExternalMemory(obj->size());
    }
    return scope.Escape(instance);
}

NAN_METHOD(Image::New)
{
	if (!info.IsConstructCall()) {
	    // [NOTE] generic recursive call with `new`
		std::vector<v8::Local<v8::Value>> args(info.Length());
		for (std::size_t i = 0; i < args.size(); ++i) args[i] = info[i];	    
	    auto inst = Nan::NewInstance(info.Callee(), args.size(), args.data());
	    if (!inst.IsEmpty()) info.GetReturnValue().Set(inst.ToLocalChecked());
	    return;
	}

    Pix *pix;
    if (info.Length() == 0) {
        pix = 0;
    } else if (info.Length() == 1 && Image::HasInstance(info[0])) {
        pix = pixCopy(NULL, Image::Pixels(info[0]->ToObject()));
    } else if (info.Length() == 2 && node::Buffer::HasInstance(info[1])) {
        String::Utf8Value format(info[0]->ToString());
        Local<Object> buffer = info[1]->ToObject();
        unsigned char *in = reinterpret_cast<unsigned char*>(node::Buffer::Data(buffer));
        size_t inLength = node::Buffer::Length(buffer);
        if (strcmp("png", *format) == 0) {
            std::vector<unsigned char> out;
            unsigned int width;
            unsigned int height;
            lodepng::State state;
            unsigned error = lodepng::decode(out, width, height, state, in, inLength);
            if (error) {
                std::stringstream msg;
                msg << "error while decoding '" << lodepng_error_text(error) << "'";
                return Nan::ThrowError(msg.str().c_str());
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
                return Nan::ThrowError("error while decoding jpg");
            }
            pix = pixFromSource(out, width, height, 32, comps == 1 ? 8 : 32);
            free(out);
        } else {
            std::stringstream msg;
            msg << "invalid bufffer format '" << *format << "'";
            return Nan::ThrowError(msg.str().c_str());
        }
    } else if (info.Length() == 3 && info[0]->IsNumber() && info[1]->IsNumber()
               && info[2]->IsNumber()) {
        int32_t width = info[0]->Int32Value();
        int32_t height = info[1]->Int32Value();
        int32_t depth = info[2]->Int32Value();
        pix = pixCreate(width, height, depth);
    } else if (info.Length() == 3 &&
               (Image::HasInstance(info[0]) || info[0]->IsNull() || info[0]->IsUndefined()) &&
               (Image::HasInstance(info[1]) || info[1]->IsNull() || info[0]->IsUndefined()) &&
               (Image::HasInstance(info[2]) || info[2]->IsNull() || info[0]->IsUndefined())) {
        pix = pixCompose(Image::Pixels(info[0]->ToObject()),
                Image::Pixels(info[1]->ToObject()),
                Image::Pixels(info[2]->ToObject()));
    } else if (info.Length() == 4 && node::Buffer::HasInstance(info[1])) {
        String::Utf8Value format(info[0]->ToString());
        Local<Object> buffer = info[1]->ToObject();
        size_t length = node::Buffer::Length(buffer);
        int32_t width = info[2]->Int32Value();
        int32_t height = info[3]->Int32Value();
        int32_t depth;
        int32_t targetDepth = 32;
        if (strcmp("rgba", *format) == 0) {
            depth = 32;
        } else if (strcmp("rgb", *format) == 0) {
            depth = 24;
        } else if (strcmp("gray", *format) == 0) {
            depth = 8;
            targetDepth = 8;
        } else {
            std::stringstream msg;
            msg << "invalid buffer format '" << *format << "'";
            return Nan::ThrowError(msg.str().c_str());
        }
        size_t expectedLength = width * height * depth;
        if (expectedLength != length << 3) {
            return Nan::ThrowError("invalid Buffer length");
        }
        pix = pixFromSource(reinterpret_cast<uint8_t*>(node::Buffer::Data(buffer)), width, height, depth, targetDepth);
    } else {
        return Nan::ThrowTypeError("expected (image: Image) or (image1: Image, "
                     "image2: Image, image3: Image) or (format: String, "
                     "image: Buffer, [width: Int32, height: Int32])");
    }
    Image* obj = new Image(pix);
    
    obj->Wrap(info.This());
}

NAN_GETTER(Image::GetWidth)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (obj->pix_) {
    	info.GetReturnValue().Set(Nan::New(obj->pix_->w));
    } else {
    	info.GetReturnValue().SetNull();
    }
}

NAN_GETTER(Image::GetHeight)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (obj->pix_) {
    	info.GetReturnValue().Set(Nan::New(obj->pix_->h));
    } else {
    	info.GetReturnValue().SetNull();
    }
}

NAN_GETTER(Image::GetDepth)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (obj->pix_) {
    	info.GetReturnValue().Set(Nan::New(obj->pix_->d));
    } else {
    	info.GetReturnValue().SetNull();
    }
}

NAN_METHOD(Image::Invert)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    Pix *pixd = pixInvert(NULL, obj->pix_);
    if (pixd == NULL) {
        return Nan::ThrowTypeError("error while applying INVERT");
    }
    info.GetReturnValue().Set(Image::New(pixd));
}

NAN_METHOD(Image::Or)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (Image::HasInstance(info[0])) {
        Pix *otherPix = Image::Pixels(info[0]->ToObject());
        Pix *pixd = pixOr(NULL, obj->pix_, otherPix);
        if (pixd == NULL) {
            return Nan::ThrowTypeError("error while applying OR");
        }
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowTypeError("expected (image: Image)");
    }
}

NAN_METHOD(Image::And)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (Image::HasInstance(info[0])) {
        Pix *otherPix = Image::Pixels(info[0]->ToObject());
        Pix *pixd = pixAnd(NULL, obj->pix_, otherPix);
        if (pixd == NULL) {
            return Nan::ThrowTypeError("error while applying AND");
        }
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowTypeError("expected (image: Image)");
    }
}

NAN_METHOD(Image::Xor)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (Image::HasInstance(info[0])) {
        Pix *otherPix = Image::Pixels(info[0]->ToObject());
        Pix *pixd = pixXor(NULL, obj->pix_, otherPix);
        if (pixd == NULL) {
            return Nan::ThrowTypeError("error while applying XOR");
        }
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowTypeError("expected (image: Image)");
    }
}

NAN_METHOD(Image::Add)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (Image::HasInstance(info[0])) {
        Pix *otherPix = Image::Pixels(info[0]->ToObject());
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
            return Nan::ThrowTypeError("error while applying ADD");
        }
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowTypeError("expected (image: Image)");
    }
}

NAN_METHOD(Image::Subtract)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (Image::HasInstance(info[0])) {
        Pix *otherPix = Image::Pixels(info[0]->ToObject());
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
            return Nan::ThrowTypeError("error while applying SUBTRACT");
        }
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowTypeError("expected (image: Image)");
    }
}

NAN_METHOD(Image::Convolve)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.This());
    if (info[0]->IsNumber() && info[1]->IsNumber()) {
        int width = static_cast<int>(ceil(info[0]->NumberValue()));
        int height = static_cast<int>(ceil(info[1]->NumberValue()));
        Pix *pixs;
        if(obj->pix_->d == 1) {
            pixs = pixConvert1To8(NULL, obj->pix_, 0, 255);
        } else {
            pixs = pixClone(obj->pix_);
        }
        Pix *pixd = pixBlockconv(pixs, width, height);
        pixDestroy(&pixs);
        if (pixd == NULL) {
            return Nan::ThrowTypeError("error while applying convolve");
        }
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowTypeError("expected (width: Number, height: Number)");
    }
}

NAN_METHOD(Image::Unsharp)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.This());
    if (info[0]->IsNumber() && info[1]->IsNumber()) {
        int halfWidth = static_cast<int>(ceil(info[0]->NumberValue()));
        float fract = static_cast<float>(info[1]->NumberValue());
        Pix *pixd = pixUnsharpMasking(obj->pix_, halfWidth, fract);
        if (pixd == NULL) {
            return Nan::ThrowTypeError("error while applying unsharp");
        }
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowTypeError("expected (halfWidth: Number, fract: Number)");
    }
}

NAN_METHOD(Image::Rotate)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.This());
    if (info[0]->IsNumber()) {
        const float deg2rad = 3.1415926535f / 180.0f;
        float angle = static_cast<float>(info[0]->NumberValue());
        Pix *pixd = pixRotate(obj->pix_, deg2rad * angle,
                              L_ROTATE_AREA_MAP, L_BRING_IN_WHITE,
                              obj->pix_->w, obj->pix_->h);
        if (pixd == NULL) {
            return Nan::ThrowTypeError("error while applying rotate");
        }
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowTypeError("expected (angle: Number)");
    }
}

NAN_METHOD(Image::Scale)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.This());
    if (info[0]->IsNumber() && (info.Length() != 2 || info[1]->IsNumber())) {
        float scaleX = static_cast<float>(info[0]->NumberValue());
        float scaleY = static_cast<float>(info.Length() == 2 ? info[1]->NumberValue() : scaleX);
        Pix *pixd = pixScale(obj->pix_, scaleX, scaleY);
        if (pixd == NULL) {
            return Nan::ThrowTypeError("error while scaling");
        }
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowTypeError("expected (scaleX: Number, [scaleY: Number])");
    }
}

NAN_METHOD(Image::Crop)
{
    Box* box = toBox(info, 0);
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.This());
    if (box) {
        PIX *pixd = pixClipRectangle(obj->pix_, box, 0);
        boxDestroy(&box);
        if (pixd == NULL) {
            return Nan::ThrowTypeError("error while cropping");
        }
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowTypeError("expected (box: Box)");
    }
}

NAN_METHOD(Image::InRange)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.This());
    if (info[0]->IsNumber() && info[1]->IsNumber() && info[2]->IsNumber() &&
            info[3]->IsNumber() && info[4]->IsNumber() && info[5]->IsNumber()) {
        int32_t val1l = info[0]->Int32Value();
        int32_t val2l = info[1]->Int32Value();
        int32_t val3l = info[2]->Int32Value();
        int32_t val1u = info[3]->Int32Value();
        int32_t val2u = info[4]->Int32Value();
        int32_t val3u = info[5]->Int32Value();
        Pix *pixd = pixInRange(obj->pix_, val1l, val2l, val3l,
                               val1u, val2u, val3u);
        if (pixd == NULL) {
            return Nan::ThrowTypeError("error while computing in range mask");
        }
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowTypeError("expected ("
                     "lower1: Number, lower2: Number, lower3: Number, "
                     "upper1: Number, upper2: Number, upper3: Number)");
    }
}

NAN_METHOD(Image::Histogram)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.This());
    PIX *mask = NULL;
    if (info.Length() >= 1 && Image::HasInstance(info[0])) {
        mask = Image::Pixels(info[1]->ToObject());
    }
    NUMA *hist = pixGetGrayHistogramMasked(obj->pix_, mask, 0, 0, obj->pix_->h > 400 ? 2 : 1);
    if (!hist) {
        return Nan::ThrowTypeError("error while computing histogram");
    }
    int len = numaGetCount(hist);
    l_float32 total = 0;
    for (int i = 0; i < len; i++) {
        total += hist->array[i];
    }
    Local<Array> result = Nan::New<Array>(len);
    for (int i = 0; i < len; i++) {
        result->Set(i, Nan::New<Number>(hist->array[i] / total));
    }
    numaDestroy(&hist);
    info.GetReturnValue().Set(result);
}

NAN_METHOD(Image::Projection)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.This());
    if (info[0]->IsString()) {
        String::Utf8Value mode(info[0]->ToString());
        ProjectionMode modeEnum;
        if (strcmp("horizontal", *mode) == 0) {
            modeEnum = Horizontal;
        }
        else if (strcmp("vertical", *mode) == 0) {
            modeEnum = Vertical;
        }
        else {
            return Nan::ThrowError("expected mode to be 'horizontal' or 'vertical'");
        }
        if (obj->pix_->d != 8 && obj->pix_->d != 1) {
            return Nan::ThrowError("expected 8bpp or 1bpp image");
        }
        std::vector<uint32_t> values;
        pixProjection(values, obj->pix_, modeEnum);
        Local<Array> result = Nan::New<Array>(static_cast<int>(values.size()));
        for (size_t i = 0; i < values.size(); i++) {
            result->Set(i, Nan::New<Number>(values[i]));
        }
        info.GetReturnValue().Set(result);
    }
    else {
        return Nan::ThrowTypeError("expected (mode: String)");
    }
}

NAN_METHOD(Image::SetMasked)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.This());
    if (Image::HasInstance(info[0]) && info[1]->IsNumber()) {
        Pix *mask = Image::Pixels(info[0]->ToObject());
        int value = info[1]->Int32Value();
        if (pixSetMasked(obj->pix_, mask, value) == 1) {
            return Nan::ThrowTypeError("error while applying mask");
        }
        info.GetReturnValue().Set(info.This());
    } else {
        return Nan::ThrowTypeError("expected (image: Image, value: Int32)");
    }
}

NAN_METHOD(Image::ApplyCurve)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.This());
    if (info[0]->IsArray() &&
            info[0]->ToObject()->Get(Nan::New("length").ToLocalChecked())->Uint32Value() == 256) {
        NUMA *numa = numaCreate(256);
        for (int i = 0; i < 256; i++) {
            numaAddNumber(numa, (l_float32)info[0]->ToObject()->Get(i)->ToInt32()->Value());
        }
        PIX *mask = NULL;
        if (info.Length() >= 2 && Image::HasInstance(info[1])) {
            mask = Image::Pixels(info[1]->ToObject());
        }
        int result = pixTRCMap(obj->pix_, mask, numa);
        if (result != 0) {
            return Nan::ThrowTypeError("error while applying value mapping");
        }
        info.GetReturnValue().Set(info.This());
    } else {
        return Nan::ThrowTypeError("expected (array: Int32[256])");
    }
}

NAN_METHOD(Image::RankFilter)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.This());
    if (info[0]->IsNumber() && info[1]->IsNumber() && info[2]->IsNumber()) {
        int width = static_cast<int>(ceil(info[0]->NumberValue()));
        int height = static_cast<int>(ceil(info[1]->NumberValue()));
        float rank = static_cast<float>(info[2]->NumberValue());
        PIX *pixd = pixRankFilter(obj->pix_, width, height, rank);
        if (pixd == NULL) {
            return Nan::ThrowTypeError("error while applying rank filter");
        }
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowTypeError("expected (width: Number, height: Number, rank: Number)");
    }
}

NAN_METHOD(Image::OctreeColorQuant)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.This());
    if (info[0]->IsInt32()) {
        int colors = info[0]->Int32Value();
        PIX *pixd = pixOctreeColorQuant(obj->pix_, colors, 0);
        if (pixd == NULL) {
            return Nan::ThrowTypeError("error while quantizating");
        }
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowTypeError("expected (colors: Int32)");
    }
}

NAN_METHOD(Image::MedianCutQuant)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.This());
    if (info[0]->IsInt32()) {
        int colors = info[0]->Int32Value();
        PIX *pixd = pixMedianCutQuantGeneral(obj->pix_, 0, 0, colors, 0, 1, 1);
        if (pixd == NULL) {
            return Nan::ThrowTypeError("error while quantizating");
        }
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowTypeError("expected (colors: Int32)");
    }
}

NAN_METHOD(Image::Threshold)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.This());
    if (info.Length() == 0 || info[0]->IsInt32()) {
        int value = info.Length() == 0 ? 128 : info[0]->Int32Value();
        PIX *pixd = pixConvertTo1(obj->pix_, value);
        if (pixd == NULL) {
            return Nan::ThrowTypeError("error while thresholding");
        }
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowTypeError("expected (value: Int32)");
    }
}

NAN_METHOD(Image::ToGray)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.This());
    if (obj->pix_->d == 8) {
        info.GetReturnValue().Set(Image::New(pixClone(obj->pix_)));
        return;
    }
    if (info.Length() == 0) {
        PIX *grayPix = pixConvertTo8(obj->pix_, 0);
        if (grayPix != NULL) {
            info.GetReturnValue().Set(Image::New(grayPix));
        } else {
            return Nan::ThrowError("error while computing grayscale image");
        }
    } else if (info.Length() == 1 && info[0]->IsString()) {
        String::Utf8Value type(info[0]->ToString());
        int32_t typeInt;
        if (strcmp("min", *type) == 0) {
            typeInt = L_CHOOSE_MIN;
        } else if (strcmp("max", *type) == 0) {
            typeInt = L_CHOOSE_MAX;
        } else {
            return Nan::ThrowError("expected type to be 'min' or 'max'");
        }
        PIX *grayPix = pixConvertRGBToGrayMinMax(
                    obj->pix_, typeInt);
        if (grayPix != NULL) {
            info.GetReturnValue().Set(Image::New(grayPix));
            return;
        } else {
            return Nan::ThrowError("error while computing grayscale image");
        }
    } else if (info.Length() == 3 && info[0]->IsNumber()
               && info[1]->IsNumber() && info[2]->IsNumber()) {
        float rwt = static_cast<float>(info[0]->NumberValue());
        float gwt = static_cast<float>(info[1]->NumberValue());
        float bwt = static_cast<float>(info[2]->NumberValue());
        PIX *grayPix = pixConvertRGBToGray(
                    obj->pix_, rwt, gwt, bwt);
        if (grayPix != NULL) {
            info.GetReturnValue().Set(Image::New(grayPix));
        } else {
            return Nan::ThrowError("error while computing grayscale image");
        }
    } else {
        return Nan::ThrowTypeError("expected (rwt: Number, gwt: Number, bwt: Number) "
                     "or (type: String) or no arguments at all");
    }
}

NAN_METHOD(Image::ToColor)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.This());
    info.GetReturnValue().Set(Image::New(pixConvertTo32(obj->pix_)));
}

NAN_METHOD(Image::ToHSV)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.This());
    PIX *pixd = pixConvertRGBToHSV(NULL, obj->pix_);
    if (pixd != NULL) {
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowError("error while converting to HSV");
    }
}

NAN_METHOD(Image::ToRGB)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.This());
    PIX *pixd = pixConvertHSVToRGB(NULL, obj->pix_);
    if (pixd != NULL) {
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowError("error while converting to RGB");
    }
}

NAN_METHOD(Image::Erode)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (info[0]->IsNumber() && info[1]->IsNumber()) {
        int width = static_cast<int>(ceil(info[0]->NumberValue()));
        int height = static_cast<int>(ceil(info[1]->NumberValue()));
        PIX *pixd = 0;
        if (obj->pix_->d == 1) {
            pixd = pixErodeBrick(NULL, obj->pix_, width, height);
        } else {
            pixd = pixErodeGray(obj->pix_, width, height);
        }
        if (pixd == NULL) {
            return Nan::ThrowTypeError("error while eroding");
        }
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowTypeError("expected (width: Number, height: Number)");
    }
}

NAN_METHOD(Image::Dilate)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (info[0]->IsNumber() && info[1]->IsNumber()) {
        int width = static_cast<int>(ceil(info[0]->NumberValue()));
        int height = static_cast<int>(ceil(info[1]->NumberValue()));
        PIX *pixd = 0;
        if (obj->pix_->d == 1) {
            pixd = pixDilateBrick(NULL, obj->pix_, width, height);
        } else {
            pixd = pixDilateGray(obj->pix_, width, height);
        }
        if (pixd == NULL) {
            return Nan::ThrowTypeError("error while dilating");
        }
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowTypeError("expected (width: Number, height: Number)");
    }
}

NAN_METHOD(Image::Open)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (info[0]->IsNumber() && info[1]->IsNumber()) {
        int width = static_cast<int>(ceil(info[0]->NumberValue()));
        int height = static_cast<int>(ceil(info[1]->NumberValue()));
        PIX *pixd = 0;
        if (obj->pix_->d == 1) {
            pixd = pixOpenBrick(NULL, obj->pix_, width, height);
        } else {
            pixd = pixOpenGray(obj->pix_, width, height);
        }
        if (pixd == NULL) {
            return Nan::ThrowTypeError("error while opening");
        }
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowTypeError("expected (width: Number, height: Number)");
    }
}

NAN_METHOD(Image::Close)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (info[0]->IsNumber() && info[1]->IsNumber()) {
        int width = static_cast<int>(ceil(info[0]->NumberValue()));
        int height = static_cast<int>(ceil(info[1]->NumberValue()));
        PIX *pixd = 0;
        if (obj->pix_->d == 1) {
            pixd = pixCloseBrick(NULL, obj->pix_, width, height);
        } else {
            pixd = pixCloseGray(obj->pix_, width, height);
        }
        if (pixd == NULL) {
            return Nan::ThrowTypeError("error while closing");
        }
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowTypeError("expected (width: Number, height: Number)");
    }
}

NAN_METHOD(Image::Thin)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (info[0]->IsString() && info[1]->IsInt32() && info[2]->IsInt32()) {
        int typeInt = 0;
        String::Utf8Value type(info[0]->ToString());
        if (strcmp("fg", *type) == 0) {
            typeInt = L_THIN_FG;
        } else if (strcmp("bg", *type) == 0) {
            typeInt = L_THIN_BG;
        }
        int connectivity = info[1]->Int32Value();
        int maxIters = info[2]->Int32Value();
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
            return Nan::ThrowTypeError("error while thinning");
        }
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowTypeError("expected (type: String, connectivity: Int32, maxIters: Int32)");
    }
}

NAN_METHOD(Image::MaxDynamicRange)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (info[0]->IsString()) {
        int typeInt = 0;
        String::Utf8Value type(info[0]->ToString());
        if (strcmp("linear", *type) == 0) {
            typeInt = L_SET_PIXELS;
        } else if (strcmp("log", *type) == 0) {
            typeInt = L_LOG_SCALE;
        }
        PIX *pixd = pixMaxDynamicRange(obj->pix_, typeInt);
        if (pixd == NULL) {
            return Nan::ThrowTypeError("error while computing max. dynamic range");
        }
        info.GetReturnValue().Set(Image::New(pixd));
    } else {
        return Nan::ThrowTypeError("expected (type: String)");
    }
}

NAN_METHOD(Image::OtsuAdaptiveThreshold)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (info[0]->IsInt32() && info[1]->IsInt32()
            && info[2]->IsInt32() && info[3]->IsInt32()
            && info[4]->IsNumber()) {
        int32_t sx = info[0]->Int32Value();
        int32_t sy = info[1]->Int32Value();
        int32_t smoothx = info[2]->Int32Value();
        int32_t smoothy = info[3]->Int32Value();
        float scorefact = static_cast<float>(info[4]->NumberValue());
        PIX *ppixth;
        PIX *ppixd;
        int error = pixOtsuAdaptiveThreshold(
                    obj->pix_, sx, sy, smoothx, smoothy,
                    scorefact, &ppixth, &ppixd);
        if (error == 0) {
            Local<Object> object = Nan::New<Object>();
            object->Set(Nan::New("thresholdValues").ToLocalChecked(), Image::New(ppixth));
            object->Set(Nan::New("image").ToLocalChecked(), Image::New(ppixd));
            info.GetReturnValue().Set(object);
            return;
        } else {
            return Nan::ThrowError("error while computing threshold");
        }
    } else {
        return Nan::ThrowTypeError("expected (sx: Int32, sy: Int32, "
                     "smoothx: Int32, smoothy: Int32, scoreFact: Number)");
    }
}

NAN_METHOD(Image::LineSegments)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (info[0]->IsInt32() && info[1]->IsInt32() && info[2]->IsBoolean()) {
        if (obj->pix_->d != 8) {
            return Nan::ThrowTypeError("Not a 8bpp Image");
        }
        int accuracy = info[0]->Int32Value();
        int maxLineSegments = info[1]->Int32Value();
        bool useWMS = info[2]->BooleanValue();
        
        if ((accuracy >= obj->pix_->w) || (accuracy >= obj->pix_->h)) {
            return Nan::ThrowError("LineSegments: Accuracy must be smaller than image");
       	}

        cv::Mat img(pix8ToMat(obj->pix_));
        std::vector<LSWMS::LSEG> lSegs;
        std::vector<double> errors;
        
        try {
        	LSWMS lswms(cv::Size(obj->pix_->w, obj->pix_->h), accuracy,
                    maxLineSegments, useWMS, false);
			lswms.run(img, lSegs, errors);
        } catch (const cv::Exception& e) {
            return Nan::ThrowError(e.what());
        }
        
        Local<Array> results = Nan::New<Array>();
        const auto strX = Nan::New("x").ToLocalChecked();
        const auto strY = Nan::New("y").ToLocalChecked();
        const auto strP1 = Nan::New("p1").ToLocalChecked();
        const auto strP2 = Nan::New("p2").ToLocalChecked();
        const auto strError = Nan::New("error").ToLocalChecked();
        for(size_t i = 0; i < lSegs.size(); i++) {
            Handle<Object> p1 = Nan::New<Object>();
            p1->Set(strX, Nan::New<Int32>(lSegs[i][0].x));
            p1->Set(strY, Nan::New<Int32>(lSegs[i][0].y));
            Handle<Object> p2 = Nan::New<Object>();
            p2->Set(strX, Nan::New<Int32>(lSegs[i][1].x));
            p2->Set(strY, Nan::New<Int32>(lSegs[i][1].y));
            Handle<Object> result = Nan::New<Object>();
            result->Set(strP1, p1);
            result->Set(strP2, p2);
            result->Set(strError, Nan::New<Number>(errors[i]));
            results->Set(i, result);
        }
        info.GetReturnValue().Set(results);
    } else {
        return Nan::ThrowTypeError("expected (accuracy: Int32, maxLineSegments: Int32, useWeightedMeanShift: Boolean)");
    }
}

NAN_METHOD(Image::FindSkew)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    int32_t depth = pixGetDepth(obj->pix_);
    if (depth != 1) {
        return Nan::ThrowTypeError("expected binarized image");
    }
    float angle;
    float conf;
    int error = pixFindSkew(obj->pix_, &angle, &conf);
    if (error == 0) {
        Local<Object> object = Nan::New<Object>();
        object->Set(Nan::New("angle").ToLocalChecked(), Nan::New(angle));
        object->Set(Nan::New("confidence").ToLocalChecked(), Nan::New(conf));
        info.GetReturnValue().Set(object);
    } else {
        return Nan::ThrowError("angle measurment not valid");
    }
}

NAN_METHOD(Image::ConnectedComponents)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (info[0]->IsInt32()) {
        int connectivity = info[0]->Int32Value();
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
            return Nan::ThrowTypeError("error while computing connected components");
        }
        Local<Object> boxes = Nan::New<Array>();
        for (int i = 0; i < boxa->n; ++i) {
            boxes->Set(i, createBox(boxa->box[i]));
        }
        boxaDestroy(&boxa);
        info.GetReturnValue().Set(boxes);
    } else {
        return Nan::ThrowTypeError("expected (connectivity: Int32)");
    }
}

NAN_METHOD(Image::DistanceFunction)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (info[0]->IsInt32()) {
        int connectivity = info[0]->Int32Value();
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
            return Nan::ThrowTypeError("error while computing distance function");
        }
        info.GetReturnValue().Set(Image::New(pixDistance));
    } else {

        return Nan::ThrowTypeError("expected (connectivity: Int32)");
    }
}

NAN_METHOD(Image::ClearBox)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    Box* box = toBox(info, 0);
    if (box) {
        int error;
        if (obj->pix_->d == 1) {
            error = pixClearInRect(obj->pix_, box);
        } else {
            error = pixSetInRect(obj->pix_, box);
        }
        boxDestroy(&box);
        if (error) {
            return Nan::ThrowTypeError("error while clearing box");
        }
        info.GetReturnValue().Set(info.This());
    } else {
        return Nan::ThrowTypeError("expected (box: Box) signature");
    }
}

NAN_METHOD(Image::FillBox)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    int boxEnd;
    BOX *box = toBox(info, 0, &boxEnd);
    if (box) {
        int error = 0;
        if (info[boxEnd + 1]->IsInt32() && !info[boxEnd + 2]->IsInt32()) {
            if (obj->pix_->d != 8 && obj->pix_->d != 1) {
                boxDestroy(&box);
                return Nan::ThrowTypeError("Not a 8bpp or 1bpp Image");
            }
            int value = info[boxEnd + 1]->Int32Value();
            error = pixSetInRectArbitrary(obj->pix_, box, value);
        }
        else if (info[boxEnd + 1]->IsInt32() && info[boxEnd + 2]->IsInt32()
                 && info[boxEnd + 3]->IsInt32()) {
            if (obj->pix_->d < 32) {
                boxDestroy(&box);
                return Nan::ThrowTypeError("Not a 32bpp Image");
            }
            uint8_t r = info[boxEnd + 1]->Int32Value();
            uint8_t g = info[boxEnd + 2]->Int32Value();
            uint8_t b = info[boxEnd + 3]->Int32Value();
            l_uint32 pixel;
            composeRGBPixel(r, g, b, &pixel);
            if (info[boxEnd + 4]->IsNumber()) {
                float fract = static_cast<float>(info[boxEnd + 4]->NumberValue());
                error = pixBlendInRect(obj->pix_, box, pixel, fract);
            }
            else {
                error = pixSetInRectArbitrary(obj->pix_, box, pixel);
            }
        }
        else {
            boxDestroy(&box);
            return Nan::ThrowTypeError("expected (box: Box, value: Int32) or "
                         "(box: Box, r: Int32, g: Int32, b: Int32, [frac: Number])");
        }
        boxDestroy(&box);
        if (error) {
            return Nan::ThrowTypeError("error while drawing box");
        }
        info.GetReturnValue().Set(info.This());
    }
    else {
        return Nan::ThrowTypeError("expected (box: Box, value: Int32) or "
                     "(box: Box, r: Int32, g: Int32, b: Int32, [frac: Number])");
    }
}

NAN_METHOD(Image::DrawBox)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    int boxEnd;
    BOX *box = toBox(info, 0, &boxEnd);
    if (box && info[boxEnd + 1]->IsInt32()) {
        int borderWidth = info[boxEnd + 1]->ToInt32()->Value();
        int error = 0;
        if (info[boxEnd + 2]->IsString()) {
            int op  = toOp(info[boxEnd + 2]);
            if (op == -1) {
                boxDestroy(&box);
                return Nan::ThrowTypeError("invalid op");
            }
            error = pixRenderBox(obj->pix_, box, borderWidth, op);
        } else if (info[boxEnd + 2]->IsInt32() && info[boxEnd + 3]->IsInt32()
                   && info[boxEnd + 4]->IsInt32()) {
            if (obj->pix_->d < 32) {
                boxDestroy(&box);
                return Nan::ThrowTypeError("Not a 32bpp Image");
            }
            uint8_t r = info[boxEnd + 2]->Int32Value();
            uint8_t g = info[boxEnd + 3]->Int32Value();
            uint8_t b = info[boxEnd + 4]->Int32Value();
            if (info[boxEnd + 5]->IsNumber()) {
                float fract = static_cast<float>(info[boxEnd + 5]->NumberValue());
                error = pixRenderBoxBlend(obj->pix_, box, borderWidth, r, g, b, fract);
            } else {
                error = pixRenderBoxArb(obj->pix_, box, borderWidth, r, g, b);
            }
        } else {
            boxDestroy(&box);
            return Nan::ThrowTypeError("expected (box: Box, borderWidth: Int32, "
                         "op: String) or (box: Box, borderWidth: Int32, r: Int32, "
                         "g: Int32, b: Int32, [frac: Number])");
        }
        boxDestroy(&box);
        if (error) {
            return Nan::ThrowTypeError("error while drawing box");
        }
        info.GetReturnValue().Set(info.This());
    } else {
        return Nan::ThrowTypeError("expected (box: Box, borderWidth: Int32, "
                     "op: String) or (box: Box, borderWidth: Int32, r: Int32, "
                     "g: Int32, b: Int32, [frac: Number])");
    }
}

NAN_METHOD(Image::DrawLine)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    const auto strX = Nan::New("x").ToLocalChecked();
    const auto strY = Nan::New("y").ToLocalChecked();
    
    if (info[0]->IsObject() && info[1]->IsObject() && info[2]->IsInt32()) {
        Handle<Object> p1 = info[0]->ToObject();
        Handle<Object> p2 = info[1]->ToObject();
        l_int32 x1 = round(p1->Get(strX)->ToNumber()->Value());
        l_int32 y1 = round(p1->Get(strY)->ToNumber()->Value());
        l_int32 x2 = round(p2->Get(strX)->ToNumber()->Value());
        l_int32 y2 = round(p2->Get(strY)->ToNumber()->Value());
        l_int32 width = info[2]->Int32Value();
        l_int32 error;
        if (info[3]->IsString()) {
            int op  = toOp(info[3]);
            if (op == -1) {
                return Nan::ThrowTypeError("invalid op");
            }
            error = pixRenderLine(obj->pix_, x1, y1, x2, y2, width, op);
        } else if (info[3]->IsInt32() && info[4]->IsInt32() && info[5]->IsInt32()) {
            if (obj->pix_->d < 32) {
                return Nan::ThrowTypeError("Not a 32bpp Image");
            }
            uint8_t r = info[3]->Int32Value();
            uint8_t g = info[4]->Int32Value();
            uint8_t b = info[5]->Int32Value();
            if (info[6]->IsNumber()) {
                float fract = static_cast<float>(info[6]->NumberValue());
                error = pixRenderLineBlend(obj->pix_, x1, y1, x2, y2, width, r, g, b, fract);
            } else {
                error = pixRenderLineArb(obj->pix_, x1, y1, x2, y2, width, r, g, b);
            }
        } else {
             return Nan::ThrowTypeError("expected (p1: Point, p2: Point, "
                          "width: Int32, op: String) or (p1: Point, p2: Point, "
                          "width: Int32, r: Int32, g: Int32, b: Int32, "
                          "[frac: Number])");
        }
        if (error) {
            return Nan::ThrowTypeError("error while drawing line");
        }
        info.GetReturnValue().Set(info.This());
    } else {
        return Nan::ThrowTypeError("expected (p1: Point, p2: Point, "
                     "width: Int32, op: String) or (p1: Point, p2: Point, "
                     "width: Int32, r: Int32, g: Int32, b: Int32, "
                     "[frac: Number])");
    }
}

NAN_METHOD(Image::DrawImage)
{
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    int boxEnd;
    BOX *box = toBox(info, 1, &boxEnd);
    if (Image::HasInstance(info[0]) && box) {
        PIX *otherPix = Image::Pixels(info[0]->ToObject());
        int error = pixRasterop(obj->pix_, box->x, box->y, box->w, box->h,
                                PIX_SRC, otherPix, 0, 0);
        boxDestroy(&box);
        if (error) {
            return Nan::ThrowTypeError("error while drawing image");
        }
        info.GetReturnValue().Set(info.This());
    } else {
        return Nan::ThrowTypeError("expected (image: Image, box: Box)");
    }
}

NAN_METHOD(Image::ToBuffer)
{
    //NanScope();
    const int FORMAT_RAW = 0;
    const int FORMAT_PNG = 1;
    const int FORMAT_JPG = 2;
    int formatInt = FORMAT_RAW;
    jpge::params params;
    Image *obj = Nan::ObjectWrap::Unwrap<Image>(info.This());
    if (info.Length() >= 1 && info[0]->IsString()) {
        String::Utf8Value format(info[0]->ToString());
        if (strcmp("raw", *format) == 0) {
            formatInt = FORMAT_RAW;
        } else if (strcmp("png", *format) == 0) {
            formatInt = FORMAT_PNG;
        } else if (strcmp("jpg", *format) == 0) {
            formatInt = FORMAT_JPG;
            if (info[1]->IsNumber()) {
                params.m_quality = info[1]->Int32Value();
            }
        } else {
            std::stringstream msg;
            msg << "invalid format '" << *format << "'";
            return Nan::ThrowError(msg.str().c_str());
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
        return Nan::ThrowError("invalid PIX depth");
    }
    if (error) {
        std::stringstream msg;
        msg << "error while encoding '" << lodepng_error_text(error) << "'";
        return Nan::ThrowError(msg.str().c_str());
    }
    if (formatInt == FORMAT_PNG) {
    	info.GetReturnValue().Set(Nan::CopyBuffer(reinterpret_cast<char *>(&pngData[0]), pngData.size()).ToLocalChecked());
    } else if (formatInt == FORMAT_JPG) {
    	info.GetReturnValue().Set(Nan::CopyBuffer(jpgData, jpgDataSize).ToLocalChecked());
        free(jpgData);
    } else {
    	info.GetReturnValue().Set(Nan::CopyBuffer(reinterpret_cast<char *>(&imgData[0]), imgData.size()).ToLocalChecked());
    }
}

Image::Image(Pix *pix)
    : pix_(pix)
{
    if (pix_) {
        Nan::AdjustExternalMemory(size());
    }
}

Image::~Image()
{
    if (pix_) {
        Nan::AdjustExternalMemory(-size());
        pixDestroy(&pix_);
    }
}

int Image::size() const
{
    return pix_->h * pix_->wpl * sizeof(uint32_t);
}

}
