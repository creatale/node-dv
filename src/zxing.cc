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
using namespace node;

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


void ZXing::Init(Handle<Object> target)
{
    Local<FunctionTemplate> constructor_template = NanNew<FunctionTemplate>(New);
    constructor_template->SetClassName(NanNew("ZXing"));
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    Local<ObjectTemplate> proto = constructor_template->PrototypeTemplate();
    proto->SetAccessor(NanNew("image"), GetImage, SetImage);
    proto->SetAccessor(NanNew("formats"), GetFormats, SetFormats);
    proto->SetAccessor(NanNew("tryHarder"), GetTryHarder, SetTryHarder);
    proto->Set(NanNew("findCode"),
               NanNew<FunctionTemplate>(FindCode)->GetFunction());
    target->Set(NanNew("ZXing"), constructor_template->GetFunction());
}

NAN_METHOD(ZXing::New)
{
    NanScope();
    Local<Object> image;
    if (args.Length() == 1 && Image::HasInstance(args[0])) {
        image = args[0]->ToObject();
    } else if (args.Length() != 0) {
        return NanThrowTypeError("cannot convert argument list to "
                     "() or "
                     "(image: Image)");
    }
    ZXing* obj = new ZXing();
    if (!image.IsEmpty()) {
        NanAssignPersistent(obj->image_, image->ToObject());
    }
    obj->Wrap(args.This());
    NanReturnThis();
}

NAN_GETTER(ZXing::GetImage)
{
    NanScope();
    ZXing* obj = ObjectWrap::Unwrap<ZXing>(args.This());
    NanReturnValue(obj->image_);
}

NAN_SETTER(ZXing::SetImage)
{
    NanScope();
    ZXing* obj = ObjectWrap::Unwrap<ZXing>(args.This());
    if (Image::HasInstance(value) || value->IsNull()) {
        if (!obj->image_.IsEmpty()) {
            NanDisposePersistent(obj->image_);
        }
        if (!value->IsNull()) {
            NanAssignPersistent(obj->image_, value->ToObject());
        }
    } else {
        return NanThrowTypeError("value must be of type Image");
    }
}

NAN_GETTER(ZXing::GetFormats)
{
    NanScope();
    ZXing* obj = ObjectWrap::Unwrap<ZXing>(args.This());
    Local<Object> format = NanNew<Object>();
    for (size_t i = 0; i < BARCODEFORMATS_LENGTH; ++i) {
        format->Set(NanNew(zxing::BarcodeFormat::barcodeFormatNames[BARCODEFORMATS[i]]),
                NanNew<Boolean>(obj->hints_.containsFormat(BARCODEFORMATS[i])));
    }
    NanReturnValue(format);
}

NAN_SETTER(ZXing::SetFormats)
{
    NanScope();
    ZXing* obj = ObjectWrap::Unwrap<ZXing>(args.This());
    if (value->IsObject()) {
        Local<Object> format = value->ToObject();
        bool tryHarder = obj->hints_.getTryHarder();
        obj->hints_.clear();
        obj->hints_.setTryHarder(tryHarder);
        for (size_t i = 0; i < BARCODEFORMATS_LENGTH; ++i) {
            if (format->Get(NanNew(zxing::BarcodeFormat::barcodeFormatNames[BARCODEFORMATS[i]]))->BooleanValue()) {
                obj->hints_.addFormat(BARCODEFORMATS[i]);
            }
        }
    } else {
        return NanThrowTypeError("value must be of type object");
    }
}

NAN_GETTER(ZXing::GetTryHarder)
{
    NanScope();
    ZXing* obj = ObjectWrap::Unwrap<ZXing>(args.This());
    NanReturnValue(NanNew<Boolean>(obj->hints_.getTryHarder()));
}

NAN_SETTER(ZXing::SetTryHarder)
{
    NanScope();
    ZXing* obj = ObjectWrap::Unwrap<ZXing>(args.This());
    if (value->IsBoolean()) {
        obj->hints_.setTryHarder(value->BooleanValue());
    } else {
        return NanThrowTypeError("value must be of type bool");
    }
}

NAN_METHOD(ZXing::FindCode)
{
    NanScope();
    ZXing* obj = ObjectWrap::Unwrap<ZXing>(args.This());
    if (obj->image_.IsEmpty()) {
        return NanThrowError("No image set");
    }
    try {
        Local<Object> image_ = NanNew<Object>(obj->image_);
        zxing::Ref<PixSource> source(new PixSource(Image::Pixels(image_)));
        zxing::Ref<zxing::Binarizer> binarizer(new zxing::HybridBinarizer(source));
        zxing::Ref<zxing::BinaryBitmap> binary(new zxing::BinaryBitmap(binarizer));
        zxing::Ref<zxing::Result> result(obj->reader_->decode(binary, obj->hints_));
        Local<Object> object = NanNew<Object>();
        std::string resultStr = result->getText()->getText();
        object->Set(NanNew("type"), NanNew<String>(zxing::BarcodeFormat::barcodeFormatNames[result->getBarcodeFormat()]));
        object->Set(NanNew("data"), NanNew<String>(resultStr.c_str()));
        object->Set(NanNew("buffer"), NanNewBufferHandle((char*)resultStr.data(), resultStr.length()));
        Local<Array> points = NanNew<Array>();
        for (int i = 0; i < result->getResultPoints()->size(); ++i) {
            Local<Object> point = NanNew<Object>();
            point->Set(NanNew("x"), NanNew<Number>(result->getResultPoints()[i]->getX()));
            point->Set(NanNew("y"), NanNew<Number>(result->getResultPoints()[i]->getY()));
            points->Set(i, point);
        }
        object->Set(NanNew("points"), points);
        NanReturnValue(object);
    } catch (const zxing::ReaderException& e) {
        if (strcmp(e.what(), "No code detected") == 0) {
            NanReturnValue(NanNull());
        } else {
            return NanThrowError(e.what());
        }
    } catch (const zxing::IllegalArgumentException& e) {
        return NanThrowError(e.what());
    } catch (const zxing::Exception& e) {
        return NanThrowError(e.what());
    } catch (const std::exception& e) {
        return NanThrowError(e.what());
    } catch (...) {
        return NanThrowError("Uncaught exception");
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
