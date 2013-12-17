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
    Local<FunctionTemplate> constructor_template = FunctionTemplate::New(New);
    constructor_template->SetClassName(String::NewSymbol("ZXing"));
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    Local<ObjectTemplate> proto = constructor_template->PrototypeTemplate();
    proto->SetAccessor(String::NewSymbol("image"), GetImage, SetImage);
    proto->SetAccessor(String::NewSymbol("formats"), GetFormats, SetFormats);
    proto->SetAccessor(String::NewSymbol("tryHarder"), GetTryHarder, SetTryHarder);
    proto->Set(String::NewSymbol("findCode"),
               FunctionTemplate::New(FindCode)->GetFunction());
    target->Set(String::NewSymbol("ZXing"),
                Persistent<Function>::New(constructor_template->GetFunction()));
}

Handle<Value> ZXing::New(const Arguments &args)
{
    HandleScope scope;
    Local<Object> image;
    if (args.Length() == 1 && Image::HasInstance(args[0])) {
        image = args[0]->ToObject();
    } else if (args.Length() != 0) {
        return THROW(TypeError, "cannot convert argument list to "
                     "() or "
                     "(image: Image)");
    }
    ZXing* obj = new ZXing();
    if (!image.IsEmpty()) {
        obj->image_ = Persistent<Object>::New(image->ToObject());
    }
    obj->Wrap(args.This());
    return args.This();
}

Handle<Value> ZXing::GetImage(Local<String> prop, const AccessorInfo &info)
{
    HandleScope scope;
    ZXing* obj = ObjectWrap::Unwrap<ZXing>(info.This());
    return scope.Close(obj->image_);
}

void ZXing::SetImage(Local<String> prop, Local<Value> value, const AccessorInfo &info)
{
    HandleScope scope;
    ZXing* obj = ObjectWrap::Unwrap<ZXing>(info.This());
    if (Image::HasInstance(value)) {
        if (!obj->image_.IsEmpty()) {
            obj->image_.Dispose();
            obj->image_.Clear();
        }
        obj->image_ = Persistent<Object>::New(value->ToObject());
    } else {
        THROW(TypeError, "value must be of type Image");
    }
}

Handle<Value> ZXing::GetFormats(Local<String> prop, const AccessorInfo &info)
{
    HandleScope scope;
    ZXing* obj = ObjectWrap::Unwrap<ZXing>(info.This());
    Local<Object> format = Object::New();
    for (size_t i = 0; i < BARCODEFORMATS_LENGTH; ++i) {
        format->Set(String::NewSymbol(zxing::BarcodeFormat::barcodeFormatNames[BARCODEFORMATS[i]]),
                Boolean::New(obj->hints_.containsFormat(BARCODEFORMATS[i])));
    }
    return scope.Close(format);
}

void ZXing::SetFormats(Local<String> prop, Local<Value> value, const AccessorInfo &info)
{
    HandleScope scope;
    ZXing* obj = ObjectWrap::Unwrap<ZXing>(info.This());
    if (value->IsObject()) {
        Local<Object> format = value->ToObject();
        bool tryHarder = obj->hints_.getTryHarder();
        obj->hints_.clear();
        obj->hints_.setTryHarder(tryHarder);
        for (size_t i = 0; i < BARCODEFORMATS_LENGTH; ++i) {
            if (format->Get(String::NewSymbol(zxing::BarcodeFormat::barcodeFormatNames[BARCODEFORMATS[i]]))->BooleanValue()) {
                obj->hints_.addFormat(BARCODEFORMATS[i]);
            }
        }
    } else {
        THROW(TypeError, "value must be of type object");
    }
}

Handle<Value> ZXing::GetTryHarder(Local<String> prop, const AccessorInfo &info)
{
    HandleScope scope;
    ZXing* obj = ObjectWrap::Unwrap<ZXing>(info.This());
    return scope.Close(Boolean::New(obj->hints_.getTryHarder()));
}

void ZXing::SetTryHarder(Local<String> prop, Local<Value> value, const AccessorInfo &info)
{
    HandleScope scope;
    ZXing* obj = ObjectWrap::Unwrap<ZXing>(info.This());
    if (value->IsBoolean()) {
        obj->hints_.setTryHarder(value->BooleanValue());
    } else {
        THROW(TypeError, "value must be of type bool");
    }
}

Handle<Value> ZXing::FindCode(const Arguments &args)
{
    HandleScope scope;
    ZXing* obj = ObjectWrap::Unwrap<ZXing>(args.This());
    if (obj->image_.IsEmpty()) {
        return THROW(Error, "No image set");
    }
    try {
        zxing::Ref<PixSource> source(new PixSource(Image::Pixels(obj->image_)));
        zxing::Ref<zxing::Binarizer> binarizer(new zxing::HybridBinarizer(source));
        zxing::Ref<zxing::BinaryBitmap> binary(new zxing::BinaryBitmap(binarizer));
        zxing::Ref<zxing::Result> result(obj->reader_->decode(binary, obj->hints_));
        Local<Object> object = Object::New();
        std::string resultStr = result->getText()->getText();
        object->Set(String::NewSymbol("type"), String::New(zxing::BarcodeFormat::barcodeFormatNames[result->getBarcodeFormat()]));
        object->Set(String::NewSymbol("data"), String::New(resultStr.c_str()));
        object->Set(String::NewSymbol("buffer"), node::Buffer::New((char*)resultStr.data(), resultStr.length())->handle_);
        Local<Array> points = Array::New();
        for (int i = 0; i < result->getResultPoints()->size(); ++i) {
            Local<Object> point = Object::New();
            point->Set(String::NewSymbol("x"), Number::New(result->getResultPoints()[i]->getX()));
            point->Set(String::NewSymbol("y"), Number::New(result->getResultPoints()[i]->getY()));
            points->Set(i, point);
        }
        object->Set(String::NewSymbol("points"), points);
        return scope.Close(object);
    } catch (const zxing::ReaderException& e) {
        if (strcmp(e.what(), "No code detected") == 0) {
            return scope.Close(Null());
        } else {
            return THROW(Error, e.what());
        }
    } catch (const zxing::IllegalArgumentException& e) {
        return THROW(Error, e.what());
    } catch (const zxing::Exception& e) {
        return THROW(Error, e.what());
    } catch (const std::exception& e) {
        return THROW(Error, e.what());
    } catch (...) {
        return THROW(Error, "Uncaught exception");
    }
}

ZXing::ZXing()
    : hints_(zxing::DecodeHints::DEFAULT_HINT), reader_(new zxing::MultiFormatReader)
{
}

ZXing::~ZXing()
{
}
