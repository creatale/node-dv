#include "tesseract.h"
#include "image.h"
#include <sstream>
#include <strngs.h>

using namespace v8;
using namespace node;

#define THROW(type, msg) \
    ThrowException(Exception::type(String::New(msg)))

void Tesseract::Init(Handle<Object> target)
{
    Local<FunctionTemplate> constructor_template = FunctionTemplate::New(New);
    constructor_template->SetClassName(String::NewSymbol("Tesseract"));
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    Local<ObjectTemplate> proto = constructor_template->PrototypeTemplate();
    proto->SetAccessor(String::NewSymbol("version"), GetVersion);
    proto->SetAccessor(String::NewSymbol("image"), GetImage, SetImage);
    proto->SetAccessor(String::NewSymbol("rectangle"), GetRectangle, SetRectangle);
    proto->SetAccessor(String::NewSymbol("pageSegMode"), GetPageSegMode, SetPageSegMode);
    proto->Set(String::NewSymbol("clear"),
               FunctionTemplate::New(Clear)->GetFunction());
    proto->Set(String::NewSymbol("clearAdaptiveClassifier"),
               FunctionTemplate::New(ClearAdaptiveClassifier)->GetFunction());
    proto->Set(String::NewSymbol("thresholdImage"),
               FunctionTemplate::New(ThresholdImage)->GetFunction());
    proto->Set(String::NewSymbol("findRegions"),
               FunctionTemplate::New(FindRegions)->GetFunction());
    proto->Set(String::NewSymbol("findTextLines"),
               FunctionTemplate::New(FindTextLines)->GetFunction());
    proto->Set(String::NewSymbol("findWords"),
               FunctionTemplate::New(FindWords)->GetFunction());
    proto->Set(String::NewSymbol("findText"),
               FunctionTemplate::New(FindText)->GetFunction());
    target->Set(String::NewSymbol("Tesseract"),
                Persistent<Function>::New(constructor_template->GetFunction()));
}

Handle<Value> Tesseract::New(const Arguments &args)
{
    HandleScope scope;
    Local<String> datapath;
    Local<String> lang;
    Local<Object> image;
    if (args.Length() == 1 && args[0]->IsString()) {
        datapath = args[0]->ToString();
        lang = String::New("eng");
    } else if (args.Length() == 2 && args[0]->IsString() && args[1]->IsString()) {
        datapath = args[0]->ToString();
        lang = args[1]->ToString();
    } else if (args.Length() == 3 && args[0]->IsString() && args[1]->IsString()
               && Image::HasInstance(args[2])) {
        datapath = args[0]->ToString();
        lang = args[1]->ToString();
        image = args[2]->ToObject();
    } else {
        return THROW(TypeError, "could not convert arguments");
    }
    Tesseract* obj = new Tesseract(*String::AsciiValue(datapath),
                                   *String::AsciiValue(lang));
    if (!image.IsEmpty()) {
        obj->image_ = Persistent<Object>::New(image->ToObject());
        obj->api_.SetImage(Image::Pixels(obj->image_));
    }
    obj->Wrap(args.This());
    return args.This();
}

Handle<Value> Tesseract::GetVersion(Local<String> prop, const AccessorInfo &info)
{
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(info.This());
    return String::New(obj->api_.Version());
}

Handle<Value> Tesseract::GetImage(Local<String> prop, const AccessorInfo &info)
{
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(info.This());
    return obj->image_;
}

void Tesseract::SetImage(Local<String> prop, Local<Value> value, const AccessorInfo &info)
{
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(info.This());
    if (Image::HasInstance(value)) {
        if (obj->image_.IsEmpty()) {
            obj->image_.Dispose();
            obj->image_.Clear();
        }
        obj->image_ = Persistent<Object>::New(value->ToObject());
        obj->api_.SetImage(Image::Pixels(obj->image_));
    }
}

Handle<Value> Tesseract::GetRectangle(Local<String> prop, const AccessorInfo &info)
{
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(info.This());
    return obj->rectangle_;
}

void Tesseract::SetRectangle(Local<String> prop, Local<Value> value, const AccessorInfo &info)
{
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(info.This());
    if (value->IsObject()) {
        if (obj->rectangle_.IsEmpty()) {
            obj->rectangle_.Dispose();
            obj->rectangle_.Clear();
        }
        obj->rectangle_ = Persistent<Object>::New(value->ToObject());
        int x = obj->rectangle_->Get(String::New("x"))->Int32Value();
        int y = obj->rectangle_->Get(String::New("y"))->Int32Value();
        int w = obj->rectangle_->Get(String::New("width"))->Int32Value();
        int h = obj->rectangle_->Get(String::New("height"))->Int32Value();
        obj->api_.SetRectangle(x, y, w, h);
    }
}

Handle<Value> Tesseract::GetPageSegMode(Local<String> prop, const AccessorInfo &info)
{
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(info.This());
    switch (obj->api_.GetPageSegMode()) {
    case tesseract::PSM_OSD_ONLY:
        return String::New("osd_only");
    case tesseract::PSM_AUTO_OSD:
        return String::New("auto_osd");
    case tesseract::PSM_AUTO_ONLY:
        return String::New("auto_only");
    case tesseract::PSM_AUTO:
        return String::New("auto");
    case tesseract::PSM_SINGLE_COLUMN:
        return String::New("single_column");
    case tesseract::PSM_SINGLE_BLOCK_VERT_TEXT:
        return String::New("single_block_vert_text");
    case tesseract::PSM_SINGLE_BLOCK:
        return String::New("single_block");
    case tesseract::PSM_SINGLE_LINE:
        return String::New("single_line");
    case tesseract::PSM_SINGLE_WORD:
        return String::New("single_word");
    case tesseract::PSM_CIRCLE_WORD:
        return String::New("circle_word");
    case tesseract::PSM_SINGLE_CHAR:
        return String::New("single_char");
    default:
        return String::New("unknown");
    }
}

void Tesseract::SetPageSegMode(Local<String> prop, Local<Value> value, const AccessorInfo &info)
{
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(info.This());
    String::AsciiValue pageSegMode(value);
    if (strcmp("osd_only", *pageSegMode)) {
        obj->api_.SetPageSegMode(tesseract::PSM_OSD_ONLY);
    } else if (strcmp("auto_osd", *pageSegMode)) {
        obj->api_.SetPageSegMode(tesseract::PSM_AUTO_OSD);
    } else if (strcmp("auto_only", *pageSegMode)) {
        obj->api_.SetPageSegMode(tesseract::PSM_AUTO_ONLY);
    } else if (strcmp("auto", *pageSegMode)) {
        obj->api_.SetPageSegMode(tesseract::PSM_AUTO);
    } else if (strcmp("single_column", *pageSegMode)) {
        obj->api_.SetPageSegMode(tesseract::PSM_SINGLE_COLUMN);
    } else if (strcmp("single_block_vert_text", *pageSegMode)) {
        obj->api_.SetPageSegMode(tesseract::PSM_SINGLE_BLOCK_VERT_TEXT);
    } else if (strcmp("single_block", *pageSegMode)) {
        obj->api_.SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
    } else if (strcmp("single_line", *pageSegMode)) {
        obj->api_.SetPageSegMode(tesseract::PSM_SINGLE_LINE);
    } else if (strcmp("single_word", *pageSegMode)) {
        obj->api_.SetPageSegMode(tesseract::PSM_SINGLE_WORD);
    } else if (strcmp("circle_word", *pageSegMode)) {
        obj->api_.SetPageSegMode(tesseract::PSM_CIRCLE_WORD);
    } else if (strcmp("single_char", *pageSegMode)) {
        obj->api_.SetPageSegMode(tesseract::PSM_SINGLE_CHAR);
    }
}

Handle<Value> Tesseract::Clear(const Arguments &args)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    obj->api_.Clear();
    return args.This();
}

Handle<Value> Tesseract::ClearAdaptiveClassifier(const Arguments &args)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    obj->api_.ClearAdaptiveClassifier();
    return args.This();
}

Handle<Value> Tesseract::ThresholdImage(const Arguments &args)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    Pix *pix = obj->api_.GetThresholdedImage();
    return scope.Close(Image::New(pix));
}

Handle<Value> Tesseract::FindRegions(const Arguments &args)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    Boxa* boxa = obj->api_.GetRegions(NULL);
    Local<Object> boxes = Array::New(boxa->n);
    for (int i = 0; i < boxa->n; ++i) {
        boxes->Set(i, createBox(boxa->box[i]));
    }
    boxaDestroy(&boxa);
    return scope.Close(boxes);
}

Handle<Value> Tesseract::FindTextLines(const Arguments &args)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    Boxa* boxa = obj->api_.GetTextlines(NULL, NULL);
    Local<Object> boxes = Array::New(boxa->n);
    for (int i = 0; i < boxa->n; ++i) {
        boxes->Set(i, createBox(boxa->box[i]));
    }
    boxaDestroy(&boxa);
    return scope.Close(boxes);
}

Handle<Value> Tesseract::FindWords(const Arguments &args)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    Boxa* boxa = obj->api_.GetWords(NULL);
    Local<Object> boxes = Array::New(boxa->n);
    for (int i = 0; i < boxa->n; ++i) {
        boxes->Set(i, createBox(boxa->box[i]));
    }
    boxaDestroy(&boxa);
    return scope.Close(boxes);
}

Handle<Value> Tesseract::FindText(const Arguments &args)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    if (args.Length() >= 1 && args[0]->IsString()) {
        String::AsciiValue format(args[0]);
        if (strcmp("plain", *format) == 0) {
            return scope.Close(String::New(obj->api_.GetUTF8Text()));
        } else if (strcmp("hocr", *format) == 0 && args.Length() == 2 && args[1]->IsInt32()) {
            return scope.Close(String::New(obj->api_.GetHOCRText(args[1]->Int32Value())));
        } else if (strcmp("box", *format) == 0 && args.Length() == 2 && args[1]->IsInt32()) {
            return scope.Close(String::New(obj->api_.GetBoxText(args[1]->Int32Value())));
        } else {
            std::stringstream msg;
            msg << "invalid format specified '" << *format << "'";
            return THROW(Error, msg.str().c_str());
        }
    } else {
        return THROW(TypeError, "could not convert arguments");
    }
}

Handle<Object> Tesseract::createBox(Box* box)
{
    Handle<Object> result = Object::New();
    result->Set(String::New("x"), Int32::New(box->x));
    result->Set(String::New("y"), Int32::New(box->y));
    result->Set(String::New("width"), Int32::New(box->w));
    result->Set(String::New("height"), Int32::New(box->h));
    return result;
}

Tesseract::Tesseract(const char *datapath, const char *language)
{
    int res = api_.Init(datapath, language, tesseract::OEM_DEFAULT,
                        NULL, NULL, NULL, NULL, false);
    assert(res == 0);
}

Tesseract::~Tesseract()
{
    api_.End();
}
