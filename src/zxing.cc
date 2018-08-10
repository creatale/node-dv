/*
 * node-dv - Document Vision for node.js
 *
 * Copyright (c) 2012 Christoph Schulz
 * Copyright (c) 2013-2015 creatale GmbH, contributors listed under AUTHORS
 * 
 * MIT License <https://github.com/creatale/node-dv/blob/master/LICENSE>
 */
#include <locale>
#include <codecvt>
#include <memory>
#include <algorithm>
#include <iostream>
#include "zxing.h"
#include "image.h"
#include "util.h"
#include <Binarizer.h>
#include <BitMatrix.h>
#include <LuminanceSource.h>
#include <MultiFormatReader.h>
#include <ReaderException.h>
#include <Result.h>
#include <HybridBinarizer.h>
#include <node_buffer.h>

using namespace v8;

namespace ZXingOriginal = ZXing;

namespace binding {

class PixSource : public ZXingOriginal::LuminanceSource
{
public:
    PixSource(Pix* pix, bool take = false);
    ~PixSource();

    virtual int width() const override;
    virtual int height() const override;
    virtual const uint8_t* getRow(int y, ZXingOriginal::ByteArray& buffer, bool forceCopy = false) const override;
    virtual const uint8_t* getMatrix(ZXingOriginal::ByteArray& buffer, int& outRowBytes, bool forceCopy = false) const override;
    virtual bool canCrop() const override;
    virtual std::shared_ptr<ZXingOriginal::LuminanceSource> cropped(int left, int top, int width, int height) const override;
    virtual bool canRotate() const override;
    virtual std::shared_ptr<ZXingOriginal::LuminanceSource> rotated(int degreeCW = 90) const override;

private:
    PIX* pix_;
};

PixSource::PixSource(Pix* pix, bool take)
    : ZXingOriginal::LuminanceSource()
{
    if (take) {
        assert(pix->d == 8);
        pix_ = pix;
    } else {
        pix_ = pixConvertTo8(pix, 0);
    }
}

PixSource::~PixSource()
{
    pixDestroy(&pix_);
}

int PixSource::width() const
{
    return pix_ ? pix_->w : 0;
}

int PixSource::height() const
{
    return pix_ ? pix_->h : 0;
}

const uint8_t* PixSource::getRow(int y, ZXingOriginal::ByteArray& buffer, bool forceCopy) const
{
    if (forceCopy) {
        std::vector<uint8_t>* row = new std::vector<uint8_t>(this->width());
        if (static_cast<uint32_t>(y) < pix_->h) {
            uint32_t *line = pix_->data + (pix_->wpl * y);
            for (int x = 0; x < this->width(); ++x) {
                (*row)[x] = GET_DATA_BYTE(line, x);
            }
        }
        return row->data();
    } else {
        buffer.resize(this->width());
        uint32_t *line = pix_->data + (pix_->wpl * y);
        for (int x = 0; x < this->width(); ++x) {
            buffer[x] = GET_DATA_BYTE(line, x);
        }
        return buffer.data();
    }
}

const uint8_t* PixSource::getMatrix(ZXingOriginal::ByteArray& buffer, int& outRowBytes, bool forceCopy) const
{
    outRowBytes = pix_->wpl;
    if (forceCopy) {
        std::vector<uint8_t>* matrix = new std::vector<uint8_t>(this->width() * this->height());
        int m = 0;
        uint32_t *line = pix_->data;
        for (uint32_t y = 0; y < pix_->h; ++y) {
            for (uint32_t x = 0; x < pix_->w; ++x) {
                (*matrix)[m] = (uint32_t)GET_DATA_BYTE(line, x);
                ++m;
            }
            line += pix_->wpl;
        }
        return matrix->data();
    } else {
        buffer.resize(this->width() * this->height());
        int m = 0;
        uint32_t *line = pix_->data;
        for (uint32_t y = 0; y < pix_->h; ++y) {
            for (uint32_t x = 0; x < pix_->w; ++x) {
                buffer[m] = (uint32_t)GET_DATA_BYTE(line, x);
                ++m;
            }
            line += pix_->wpl;
        }
        return buffer.data();
    }
}

bool PixSource::canCrop() const
{
    return true;
}

std::shared_ptr<ZXingOriginal::LuminanceSource> PixSource::cropped(int left, int top, int width, int height) const
{
    BOX *box = boxCreate(left, top, width, height);
    PIX *croppedPix = pixClipRectangle(pix_, box, 0);
    boxDestroy(&box);
    return std::shared_ptr<PixSource>(new PixSource(croppedPix, true));
}

bool PixSource::canRotate() const
{
    return true;
}

std::shared_ptr<ZXingOriginal::LuminanceSource> PixSource::rotated(int degreeCW) const
{
    // Rotate 90 degree counterclockwise.
    if (pix_->w != 0 && pix_->h != 0) {
        Pix *rotatedPix = pixRotate90(pix_, -1);
        return std::shared_ptr<PixSource>(new PixSource(rotatedPix, true));
    } else {
        return std::shared_ptr<PixSource>(new PixSource(pix_));
    }
}

const ZXingOriginal::BarcodeFormat ZXing::BARCODEFORMATS[] = {
    ZXingOriginal::BarcodeFormat::QR_CODE,
    ZXingOriginal::BarcodeFormat::DATA_MATRIX,
    ZXingOriginal::BarcodeFormat::PDF_417,
    ZXingOriginal::BarcodeFormat::UPC_E,
    ZXingOriginal::BarcodeFormat::UPC_A,
    ZXingOriginal::BarcodeFormat::EAN_8,
    ZXingOriginal::BarcodeFormat::EAN_13,
    ZXingOriginal::BarcodeFormat::CODE_128,
    ZXingOriginal::BarcodeFormat::CODE_39,
    ZXingOriginal::BarcodeFormat::ITF,
    ZXingOriginal::BarcodeFormat::AZTEC
};

const size_t ZXing::BARCODEFORMATS_LENGTH = 11;



NAN_MODULE_INIT(ZXing::Init)
{
    auto ctor = Nan::New<v8::FunctionTemplate>(New);
    auto ctorInst = ctor->InstanceTemplate();
    auto name = Nan::New("ZXing").ToLocalChecked();
    ctor->SetClassName(name);
    ctorInst->SetInternalFieldCount(1);

    Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
    Nan::SetAccessor(proto, Nan::New("image").ToLocalChecked(), GetImage, SetImage);
    Nan::SetAccessor(proto, Nan::New("formats").ToLocalChecked(), GetFormats, SetFormats);
    Nan::SetAccessor(proto, Nan::New("tryHarder").ToLocalChecked(), GetTryHarder, SetTryHarder);
    
    Nan::SetPrototypeMethod(ctor, "findCode", FindCode);
    Nan::Set(target, name, ctor->GetFunction());
}

NAN_METHOD(ZXing::New)
{
    if (!info.IsConstructCall()) {
        std::vector<v8::Local<v8::Value>> args(info.Length());
        for (std::size_t i = 0; i < args.size(); ++i) args[i] = info[i];        
        auto inst = Nan::NewInstance(info.Callee(), args.size(), args.data());
        if (!inst.IsEmpty()) info.GetReturnValue().Set(inst.ToLocalChecked());
        return;
    }

    Local<Object> image;
    if (info.Length() == 1 && Image::HasInstance(info[0])) {
        image = info[0]->ToObject();
    } else if (info.Length() != 0) {
        return Nan::ThrowTypeError("cannot convert argument list to "
                     "() or "
                     "(image: Image)");
    }
    ZXing* obj = new ZXing();
    if (!image.IsEmpty()) {
        obj->image_.Reset(image->ToObject());
    }
    obj->Wrap(info.This());
}

NAN_GETTER(ZXing::GetImage)
{
    ZXing* obj = Nan::ObjectWrap::Unwrap<ZXing>(info.This());
    info.GetReturnValue().Set(Nan::New(obj->image_));
}

NAN_SETTER(ZXing::SetImage)
{
    ZXing* obj = Nan::ObjectWrap::Unwrap<ZXing>(info.This());
    if (Image::HasInstance(value) || value->IsNull()) {
        if (!obj->image_.IsEmpty()) {
            obj->image_.Reset();
        }
        if (!value->IsNull()) {
            obj->image_.Reset(value->ToObject());
        }
    } else {
        Nan::ThrowTypeError("value must be of type Image");
    }
}

NAN_GETTER(ZXing::GetFormats)
{
    ZXing* obj = Nan::ObjectWrap::Unwrap<ZXing>(info.This());
    Local<Object> format = Nan::New<Object>();
    std::vector<ZXingOriginal::BarcodeFormat> barcodeFormats = obj->hints_.possibleFormats();
    for (size_t i = 0; i < BARCODEFORMATS_LENGTH; ++i) {
        auto name = Nan::New(ZXingOriginal::ToString(BARCODEFORMATS[i])).ToLocalChecked();
        std::vector<ZXingOriginal::BarcodeFormat>::iterator found = std::find(barcodeFormats.begin(), barcodeFormats.end(), BARCODEFORMATS[i]);
        format->Set(name, Nan::New<Boolean>(found != barcodeFormats.end()));
    }
    info.GetReturnValue().Set(format);
}

NAN_SETTER(ZXing::SetFormats)
{
    ZXing* obj = Nan::ObjectWrap::Unwrap<ZXing>(info.This());
    if (value->IsObject()) {
        Local<Object> format = value->ToObject();
        std::vector<ZXingOriginal::BarcodeFormat> formats;
        for (size_t i = 0; i < BARCODEFORMATS_LENGTH; ++i) {
            auto name = Nan::New(ZXingOriginal::ToString(BARCODEFORMATS[i])).ToLocalChecked();
            if (format->Get(name)->BooleanValue()) {
                formats.push_back(BARCODEFORMATS[i]);
            }
        }
        obj->hints_.setPossibleFormats(formats);
    } else {
        Nan::ThrowTypeError("value must be of type object");
    }
}

NAN_GETTER(ZXing::GetTryHarder)
{
    ZXing* obj = Nan::ObjectWrap::Unwrap<ZXing>(info.This());
    info.GetReturnValue().Set(Nan::New<Boolean>(obj->hints_.shouldTryHarder()));
}

NAN_SETTER(ZXing::SetTryHarder)
{
    ZXing* obj = Nan::ObjectWrap::Unwrap<ZXing>(info.This());
    if (value->IsBoolean()) {
        obj->hints_.setShouldTryHarder(value->BooleanValue());
    } else {
        Nan::ThrowTypeError("value must be of type bool");
    }
}

NAN_METHOD(ZXing::FindCode)
{
    ZXing* obj = Nan::ObjectWrap::Unwrap<ZXing>(info.This());
    if (obj->image_.IsEmpty()) {
        return Nan::ThrowError("No image set");
    }
    try {
        Local<Object> image_ = Nan::New<Object>(obj->image_);
        std::shared_ptr<PixSource> source(new PixSource(Image::Pixels(image_)));
        std::shared_ptr<ZXingOriginal::HybridBinarizer> binarizer(new ZXingOriginal::HybridBinarizer(source));
        ZXingOriginal::Result result = obj->reader_->read(*binarizer);
        Local<Object> object = Nan::New<Object>();
        std::wstring resultWStr = result.text();
        
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        std::string resultStr = converter.to_bytes(resultWStr);
        
        ZXingOriginal::BarcodeFormat format = result.format();
        if (format == ZXingOriginal::BarcodeFormat::FORMAT_COUNT) {
            info.GetReturnValue().Set(Nan::Null());
        } else {
            object->Set(Nan::New("type").ToLocalChecked(),
                    Nan::New<String>(ZXingOriginal::ToString(result.format())).ToLocalChecked());
            object->Set(Nan::New("data").ToLocalChecked(),
                    Nan::New<String>(resultStr.c_str()).ToLocalChecked());
            object->Set(Nan::New("buffer").ToLocalChecked(),
                    Nan::NewBuffer((char*)resultStr.data(), resultStr.length()).ToLocalChecked());
            Local<Array> points = Nan::New<Array>();
            auto strX = Nan::New("x").ToLocalChecked();
            auto strY = Nan::New("y").ToLocalChecked();
            for (int i = 0; i < result.resultPoints().size(); ++i) {
                Local<Object> point = Nan::New<Object>();
                point->Set(strX, Nan::New<Number>(result.resultPoints()[i].x()));
                point->Set(strY, Nan::New<Number>(result.resultPoints()[i].y()));
                points->Set(i, point);
            }
            object->Set(Nan::New("points").ToLocalChecked(), points);
            info.GetReturnValue().Set(object);
        }
    } catch (const ZXingOriginal::ReaderException& e) {
        if (strcmp(e.what(), "No code detected") == 0) {
            info.GetReturnValue().Set(Nan::Null());
            return;
        } else {
            return Nan::ThrowError(e.what());
        }
    } catch (const ZXingOriginal::Exception& e) {
        return Nan::ThrowError(e.what());
    } catch (const std::exception& e) {
        return Nan::ThrowError(e.what());
    } catch (...) {
        return Nan::ThrowError("Uncaught exception");
    }
}

ZXing::ZXing()
    : hints_()
{
    this->reader_ = std::shared_ptr<ZXingOriginal::MultiFormatReader>(new ZXingOriginal::MultiFormatReader(this->hints_));
}

ZXing::~ZXing()
{
}

}
