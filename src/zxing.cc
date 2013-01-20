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
#include <zxing/LuminanceSource.h>
#include <zxing/Binarizer.h>
#include <zxing/BinaryBitmap.h>
#include <zxing/common/HybridBinarizer.h>
#include <zxing/common/HybridBinarizer.h>
#include <zxing/Result.h>
#include <zxing/ReaderException.h>
#include <zxing/MultiFormatReader.h>
#include <zxing/multi/GenericMultipleBarcodeReader.h>
#include <sstream>

using namespace v8;
using namespace node;

class PixSource : public zxing::LuminanceSource
{
public:
    PixSource(Pix* pix, bool take = false);
    ~PixSource();

    int getWidth() const;
    int getHeight() const;
    unsigned char* getRow(int y, unsigned char* row);
    unsigned char* getMatrix();

    bool isCropSupported() const;
    zxing::Ref<zxing::LuminanceSource> crop(int left, int top, int width, int height);
    bool isRotateSupported() const;
    zxing::Ref<zxing::LuminanceSource> rotateCounterClockwise();

private:
    PIX* pix_;
};

PixSource::PixSource(Pix* pix, bool take)
{
    assert(pix->d == 8);
    if (take) {
        pix_ = pix;
    } else {
        pix_ = pixClone(pix);
    }
}

PixSource::~PixSource()
{
    pixDestroy(&pix_);
}

int PixSource::getWidth() const
{
    return pix_->w;
}

int PixSource::getHeight() const
{
    return pix_->h;
}

unsigned char* PixSource::getRow(int y, unsigned char* row)
{
    row = row ? row : new unsigned char[pix_->w];
    if (static_cast<uint32_t>(y) < pix_->h) {
        uint32_t *line = pix_->data + (pix_->wpl * y);
        unsigned char *r = row;
        for (uint32_t x = 0; x < pix_->w; ++x) {
            *r = GET_DATA_BYTE(line, x);
            ++r;
        }
    }
    return row;
}

unsigned char* PixSource::getMatrix()
{
    unsigned char* matrix = new unsigned char[pix_->w * pix_->h];
    unsigned char* m = matrix;
    uint32_t *line = pix_->data;
    for (uint32_t y = 0; y < pix_->h; ++y) {
        for (uint32_t x = 0; x < pix_->w; ++x) {
            *m = GET_DATA_BYTE(line, x);
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

zxing::Ref<zxing::LuminanceSource> PixSource::rotateCounterClockwise()
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

zxing::Ref<zxing::LuminanceSource> PixSource::crop(int left, int top, int width, int height)
{
    BOX *box = boxCreate(left, top, width, height);
    PIX *croppedPix = pixClipRectangle(pix_, box, 0);
    boxDestroy(&box);
    return zxing::Ref<PixSource>(new PixSource(croppedPix, true));
}

void ZXing::Init(Handle<Object> target)
{
    Local<FunctionTemplate> constructor_template = FunctionTemplate::New(New);
    constructor_template->SetClassName(String::NewSymbol("ZXing"));
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    Local<ObjectTemplate> proto = constructor_template->PrototypeTemplate();
    proto->SetAccessor(String::NewSymbol("image"), GetImage, SetImage);
    proto->SetAccessor(String::NewSymbol("tryHarder"), GetTryHarder, SetTryHarder);
    proto->Set(String::NewSymbol("findCode"),
               FunctionTemplate::New(FindCode)->GetFunction());
    proto->Set(String::NewSymbol("findCodes"),
               FunctionTemplate::New(FindCodes)->GetFunction());
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
    ZXing* obj = ObjectWrap::Unwrap<ZXing>(info.This());
    return obj->image_;
}

void ZXing::SetImage(Local<String> prop, Local<Value> value, const AccessorInfo &info)
{
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

Handle<Value> ZXing::GetTryHarder(Local<String> prop, const AccessorInfo &info)
{
    ZXing* obj = ObjectWrap::Unwrap<ZXing>(info.This());
    return Boolean::New(obj->hints_.getTryHarder());
}

void ZXing::SetTryHarder(Local<String> prop, Local<Value> value, const AccessorInfo &info)
{
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
    try {
        zxing::Ref<PixSource> source(new PixSource(Image::Pixels(obj->image_)));
        zxing::Ref<zxing::Binarizer> binarizer(new zxing::HybridBinarizer(source));
        zxing::Ref<zxing::BinaryBitmap> binary(new zxing::BinaryBitmap(binarizer));
        zxing::Ref<zxing::Result> result(obj->reader_->decodeWithState(binary));
        Local<Object> object = Object::New();
        object->Set(String::NewSymbol("type"), String::New(zxing::barcodeFormatNames[result->getBarcodeFormat()]));
        object->Set(String::NewSymbol("data"), String::New(result->getText()->getText().c_str()));
        Local<Array> points = Array::New();
        for (size_t i = 0; i < result->getResultPoints().size(); ++i) {
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

Handle<Value> ZXing::FindCodes(const Arguments &args)
{
    HandleScope scope;
    ZXing* obj = ObjectWrap::Unwrap<ZXing>(args.This());
    try {
        zxing::Ref<PixSource> source(new PixSource(Image::Pixels(obj->image_)));
        zxing::Ref<zxing::Binarizer> binarizer(new zxing::HybridBinarizer(source));
        zxing::Ref<zxing::BinaryBitmap> binary(new zxing::BinaryBitmap(binarizer));
        std::vector< zxing::Ref<zxing::Result> > result = obj->multiReader_->decodeMultiple(binary, obj->hints_);
        Local<Array> objects = Array::New();
        for (size_t i = 0; i < result.size(); ++i) {
            Local<Object> object = Object::New();
            object->Set(String::NewSymbol("type"), String::New(zxing::barcodeFormatNames[result[i]->getBarcodeFormat()]));
            object->Set(String::NewSymbol("data"), String::New(result[i]->getText()->getText().c_str()));
            Local<Array> points = Array::New();
            for (size_t j = 0; j < result[i]->getResultPoints().size(); ++j) {
                Local<Object> point = Object::New();
                point->Set(String::NewSymbol("x"), Number::New(result[i]->getResultPoints()[j]->getX()));
                point->Set(String::NewSymbol("y"), Number::New(result[i]->getResultPoints()[j]->getY()));
                points->Set(j, point);
            }
            object->Set(String::NewSymbol("points"), points);
            objects->Set(i, object);
        }
        return scope.Close(objects);
    } catch (const zxing::ReaderException& e) {
        if (strcmp(e.what(), "No code detected") == 0) {
            return scope.Close(Array::New());
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
    : hints_(zxing::DecodeHints::DEFAULT_HINT), reader_(new zxing::MultiFormatReader),
      multiReader_(new zxing::multi::GenericMultipleBarcodeReader(*reader_))
{
    //TODO: implement getters/setters for hints
    //hints_.addFormat(...)
    reader_->setHints(hints_);
}

ZXing::~ZXing()
{
}
