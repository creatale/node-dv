/*
 * node-dv - Document Vision for node.js
 *
 * Copyright (c) 2012 Christoph Schulz
 * Copyright (c) 2013-2015 creatale GmbH, contributors listed under AUTHORS
 * 
 * MIT License <https://github.com/creatale/node-dv/blob/master/LICENSE>
 */
#include "zxing.h"
#include "image.h"
#include "util.h"
#include <zxing/Binarizer.h>
#include <zxing/BinaryBitmap.h>
#include <zxing/LuminanceSource.h>
#include <zxing/MultiFormatReader.h>
#include <zxing/ReaderException.h>
#include <zxing/Result.h>
#include <zxing/common/Array.h>
#include <zxing/common/HybridBinarizer.h>
#include <zxing/multi/GenericMultipleBarcodeReader.h>
#include <node_buffer.h>

using namespace v8;

namespace binding {

class PixSource : public zxing::LuminanceSource
{
public:
    PixSource(Pix* pix, bool take = false);
    ~PixSource();

    zxing::ArrayRef<char> getRow(int y, zxing::ArrayRef<char> row) const;
    zxing::ArrayRef<char> getMatrix() const;

    bool isCropSupported() const;
    zxing::Ref<zxing::LuminanceSource> crop(int left, int top, int width, int height) const;

    bool isRotateSupported() const;
    zxing::Ref<zxing::LuminanceSource> rotateCounterClockwise() const;

private:
    PIX* pix_;
};

PixSource::PixSource(Pix* pix, bool take)
    : LuminanceSource(pix ? pix->w : 0, pix ? pix->h : 0)
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

zxing::ArrayRef<char> PixSource::getRow(int y, zxing::ArrayRef<char> row) const
{
    if (!row) {
        row = zxing::ArrayRef<char>(getWidth());
    }
    if (static_cast<uint32_t>(y) < pix_->h) {
        uint32_t *line = pix_->data + (pix_->wpl * y);
        for (int x = 0; x < getWidth(); ++x) {
            row[x] = GET_DATA_BYTE(line, x);
        }
    }
    return row;
}

zxing::ArrayRef<char> PixSource::getMatrix() const
{
    zxing::ArrayRef<char> matrix(getWidth() * getHeight());
    char* m = &matrix[0];
    uint32_t *line = pix_->data;
    for (uint32_t y = 0; y < pix_->h; ++y) {
        for (uint32_t x = 0; x < pix_->w; ++x) {
            *m = (char)GET_DATA_BYTE(line, x);
            ++m;
        }
        line += pix_->wpl;
    }
    return matrix;
}

bool PixSource::isRotateSupported() const
{
    return true;
}

zxing::Ref<zxing::LuminanceSource> PixSource::rotateCounterClockwise() const
{
    // Rotate 90 degree counterclockwise.
    if (pix_->w != 0 && pix_->h != 0) {
        Pix *rotatedPix = pixRotate90(pix_, -1);
        return zxing::Ref<PixSource>(new PixSource(rotatedPix, true));
    } else {
        return zxing::Ref<PixSource>(new PixSource(pix_));
    }
}

bool PixSource::isCropSupported() const
{
    return true;
}

zxing::Ref<zxing::LuminanceSource> PixSource::crop(int left, int top, int width, int height) const
{
    BOX *box = boxCreate(left, top, width, height);
    PIX *croppedPix = pixClipRectangle(pix_, box, 0);
    boxDestroy(&box);
    return zxing::Ref<PixSource>(new PixSource(croppedPix, true));
}

const zxing::BarcodeFormat::Value ZXing::BARCODEFORMATS[] = {
    zxing::BarcodeFormat::QR_CODE,
    zxing::BarcodeFormat::DATA_MATRIX,
    zxing::BarcodeFormat::PDF_417,
    zxing::BarcodeFormat::UPC_E,
    zxing::BarcodeFormat::UPC_A,
    zxing::BarcodeFormat::EAN_8,
    zxing::BarcodeFormat::EAN_13,
    zxing::BarcodeFormat::CODE_128,
    zxing::BarcodeFormat::CODE_39,
    zxing::BarcodeFormat::ITF,
    zxing::BarcodeFormat::AZTEC
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
    for (size_t i = 0; i < BARCODEFORMATS_LENGTH; ++i) {
    	auto name = Nan::New(zxing::BarcodeFormat::barcodeFormatNames[BARCODEFORMATS[i]]).ToLocalChecked();
        format->Set(name, Nan::New<Boolean>(obj->hints_.containsFormat(BARCODEFORMATS[i])));
    }
    info.GetReturnValue().Set(format);
}

NAN_SETTER(ZXing::SetFormats)
{
    ZXing* obj = Nan::ObjectWrap::Unwrap<ZXing>(info.This());
    if (value->IsObject()) {
        Local<Object> format = value->ToObject();
        bool tryHarder = obj->hints_.getTryHarder();
        obj->hints_.clear();
        obj->hints_.setTryHarder(tryHarder);
        for (size_t i = 0; i < BARCODEFORMATS_LENGTH; ++i) {
        	auto name = Nan::New(zxing::BarcodeFormat::barcodeFormatNames[BARCODEFORMATS[i]]).ToLocalChecked();
            if (format->Get(name)->BooleanValue()) {
                obj->hints_.addFormat(BARCODEFORMATS[i]);
            }
        }
    } else {
        Nan::ThrowTypeError("value must be of type object");
    }
}

NAN_GETTER(ZXing::GetTryHarder)
{
    ZXing* obj = Nan::ObjectWrap::Unwrap<ZXing>(info.This());
    info.GetReturnValue().Set(Nan::New<Boolean>(obj->hints_.getTryHarder()));
}

NAN_SETTER(ZXing::SetTryHarder)
{
    ZXing* obj = Nan::ObjectWrap::Unwrap<ZXing>(info.This());
    if (value->IsBoolean()) {
        obj->hints_.setTryHarder(value->BooleanValue());
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
        zxing::Ref<PixSource> source(new PixSource(Image::Pixels(image_)));
        zxing::Ref<zxing::Binarizer> binarizer(new zxing::HybridBinarizer(source));
        zxing::Ref<zxing::BinaryBitmap> binary(new zxing::BinaryBitmap(binarizer));
        zxing::Ref<zxing::Result> result(obj->reader_->decode(binary, obj->hints_));
        Local<Object> object = Nan::New<Object>();
        std::string resultStr = result->getText()->getText();
        object->Set(Nan::New("type").ToLocalChecked(),
        		Nan::New<String>(zxing::BarcodeFormat::barcodeFormatNames[result->getBarcodeFormat()]).ToLocalChecked());
        object->Set(Nan::New("data").ToLocalChecked(),
        		Nan::New<String>(resultStr.c_str()).ToLocalChecked());
        object->Set(Nan::New("buffer").ToLocalChecked(),
        		Nan::CopyBuffer((char*)resultStr.data(), resultStr.length()).ToLocalChecked());
        Local<Array> points = Nan::New<Array>();
        auto strX = Nan::New("x").ToLocalChecked();
        auto strY = Nan::New("y").ToLocalChecked();
        for (int i = 0; i < result->getResultPoints()->size(); ++i) {
            Local<Object> point = Nan::New<Object>();
            point->Set(strX, Nan::New<Number>(result->getResultPoints()[i]->getX()));
            point->Set(strY, Nan::New<Number>(result->getResultPoints()[i]->getY()));
            points->Set(i, point);
        }
        object->Set(Nan::New("points").ToLocalChecked(), points);
        info.GetReturnValue().Set(object);
    } catch (const zxing::ReaderException& e) {
        if (strcmp(e.what(), "No code detected") == 0) {
            info.GetReturnValue().Set(Nan::Null());
            return;
        } else {
            return Nan::ThrowError(e.what());
        }
    } catch (const zxing::IllegalArgumentException& e) {
        return Nan::ThrowError(e.what());
    } catch (const zxing::Exception& e) {
        return Nan::ThrowError(e.what());
    } catch (const std::exception& e) {
        return Nan::ThrowError(e.what());
    } catch (...) {
        return Nan::ThrowError("Uncaught exception");
    }
}

ZXing::ZXing()
    : hints_(zxing::DecodeHints::DEFAULT_HINT), reader_(new zxing::MultiFormatReader)
{
}

ZXing::~ZXing()
{
}

}
