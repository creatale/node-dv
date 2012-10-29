#include "zbar.h"
#include "image.h"
#include <sstream>

using namespace v8;
using namespace node;

#define THROW(type, msg) \
    ThrowException(Exception::type(String::New(msg)))

void ZBar::Init(Handle<Object> target)
{
    Local<FunctionTemplate> constructor_template = FunctionTemplate::New(New);
    constructor_template->SetClassName(String::NewSymbol("ZBar"));
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    Local<ObjectTemplate> proto = constructor_template->PrototypeTemplate();
    proto->SetAccessor(String::NewSymbol("image"), GetImage, SetImage);
    proto->Set(String::NewSymbol("findSymbols"),
               FunctionTemplate::New(FindSymbols)->GetFunction());
    target->Set(String::NewSymbol("ZBar"),
                Persistent<Function>::New(constructor_template->GetFunction()));
}

Handle<Value> ZBar::New(const Arguments &args)
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
    ZBar* obj = new ZBar();
    if (!image.IsEmpty()) {
        obj->image_ = Persistent<Object>::New(image->ToObject());
        obj->SetZImage(obj->image_);
    }
    obj->Wrap(args.This());
    return args.This();
}

Handle<Value> ZBar::GetImage(Local<String> prop, const AccessorInfo &info)
{
    ZBar* obj = ObjectWrap::Unwrap<ZBar>(info.This());
    return obj->image_;
}

void ZBar::SetImage(Local<String> prop, Local<Value> value, const AccessorInfo &info)
{
    ZBar* obj = ObjectWrap::Unwrap<ZBar>(info.This());
    if (Image::HasInstance(value)) {
        if (obj->image_.IsEmpty()) {
            obj->image_.Dispose();
            obj->image_.Clear();
        }
        obj->image_ = Persistent<Object>::New(value->ToObject());
        obj->SetZImage(obj->image_);
    } else {
        THROW(TypeError, "value must be of type Image");
    }
}

Handle<Value> ZBar::FindSymbols(const Arguments &args)
{
    HandleScope scope;
    ZBar* obj = ObjectWrap::Unwrap<ZBar>(args.This());
    int count = zbar_scan_image(obj->zScanner_, obj->zImage_);
    Local<Array> symbols = Array::New(count);
    // extract results
    int index = 0;
    for (const zbar_symbol_t *symbol = zbar_image_first_symbol(obj->zImage_);
         symbol; symbol = zbar_symbol_next(symbol)) {
        zbar_symbol_type_t type = zbar_symbol_get_type(symbol);
        Local<Object> object = Object::New();
        object->Set(String::NewSymbol("type"), String::New(zbar_get_symbol_name(type)));
        object->Set(String::NewSymbol("data"), String::New(zbar_symbol_get_data(symbol)));
        symbols->Set(index, object);
        ++index;
    }
    return scope.Close(symbols);
}

ZBar::ZBar()
    : zScanner_(0), zImage_(0)
{
    zScanner_ = zbar_image_scanner_create();
    zImage_ = zbar_image_create();
}

ZBar::~ZBar()
{
    zbar_image_destroy(zImage_);
    zbar_image_scanner_destroy(zScanner_);
}

void zbar_delete_data(zbar_image_t *image)
{
    delete[] static_cast<const char*>(zbar_image_get_data(image));
}

void ZBar::SetZImage(Handle<Object> image)
{
    Pix *pix = Image::Pixels(image);
    if (pix->d != 8) {
        THROW(Error, "image must have grayscale format");
        return;
    }
    // Create a copy with one byte per pixel.
    char *raw = new char[pix->w * pix->h];
    char *rawIter = raw;
    uint32_t *line = pix->data;
    for (uint32_t y = 0; y < pix->h; ++y) {
        for (uint32_t x = 0; x < pix->w; ++x) {
            *rawIter = GET_DATA_BYTE(line, x);
            ++rawIter;
        }
    }
    zbar_image_set_format(zImage_, *(int*)"Y800");
    zbar_image_set_size(zImage_, pix->w, pix->h);
    zbar_image_set_data(zImage_, raw, pix->w * pix->h, &zbar_delete_data);
}
