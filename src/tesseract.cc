/*
 * node-dv - Document Vision for node.js
 *
 * Copyright (c) 2012 Christoph Schulz
 * Copyright (c) 2013-2015 creatale GmbH, contributors listed under AUTHORS
 * 
 * MIT License <https://github.com/creatale/node-dv/blob/master/LICENSE>
 */
#include "tesseract.h"
#include "image.h"
#include "util.h"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <strngs.h>
#include <resultiterator.h>
#include <tesseractclass.h>
#include <params.h>

using namespace v8;

namespace binding {

#define ReturnValue(value) return info.GetReturnValue().Set(Nan::New(value).ToLocalChecked())

NAN_MODULE_INIT(Tesseract::Init)
{
    Local<FunctionTemplate> constructor_template = Nan::New<FunctionTemplate>(New);
    constructor_template->SetClassName(Nan::New("Tesseract").ToLocalChecked());
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    Local<ObjectTemplate> proto = constructor_template->PrototypeTemplate();
    
    Nan::SetAccessor(proto, Nan::New("image").ToLocalChecked(), GetImage, SetImage);
    Nan::SetAccessor(proto, Nan::New("rectangle").ToLocalChecked(), GetRectangle, SetRectangle);
    Nan::SetAccessor(proto, Nan::New("pageSegMode").ToLocalChecked(), GetPageSegMode, SetPageSegMode);
    Nan::SetAccessor(proto, Nan::New("symbolWhitelist").ToLocalChecked(), GetSymbolWhitelist, SetSymbolWhitelist); //TODO: remove (deprecated).
    
    tesseract::Tesseract* tesseract_ = new tesseract::Tesseract;
    GenericVector<tesseract::IntParam *> global_int_vec = GlobalParams()->int_params;
    GenericVector<tesseract::IntParam *> member_int_vec = tesseract_->params()->int_params;
    GenericVector<tesseract::BoolParam *> global_bool_vec = GlobalParams()->bool_params;
    GenericVector<tesseract::BoolParam *> member_bool_vec = tesseract_->params()->bool_params;
    GenericVector<tesseract::DoubleParam *> global_double_vec = GlobalParams()->double_params;
    GenericVector<tesseract::DoubleParam *> member_double_vec = tesseract_->params()->double_params;
    GenericVector<tesseract::StringParam *> global_string_vec = GlobalParams()->string_params;
    GenericVector<tesseract::StringParam *> member_string_vec = tesseract_->params()->string_params;
    for (int i = 0; i < global_int_vec.size(); ++i)
        Nan::SetAccessor(proto, Nan::New(global_int_vec[i]->name_str()).ToLocalChecked(), GetIntVariable, SetVariable);
    for (int i = 0; i < member_int_vec.size(); ++i)
        Nan::SetAccessor(proto, Nan::New(member_int_vec[i]->name_str()).ToLocalChecked(), GetIntVariable, SetVariable);
    for (int i = 0; i < global_bool_vec.size(); ++i)
        Nan::SetAccessor(proto, Nan::New(global_bool_vec[i]->name_str()).ToLocalChecked(), GetBoolVariable, SetVariable);
    for (int i = 0; i < member_bool_vec.size(); ++i)
        Nan::SetAccessor(proto, Nan::New(member_bool_vec[i]->name_str()).ToLocalChecked(), GetBoolVariable, SetVariable);
    for (int i = 0; i < global_double_vec.size(); ++i)
        Nan::SetAccessor(proto, Nan::New(global_double_vec[i]->name_str()).ToLocalChecked(), GetDoubleVariable, SetVariable);
    for (int i = 0; i < member_double_vec.size(); ++i)
        Nan::SetAccessor(proto, Nan::New(member_double_vec[i]->name_str()).ToLocalChecked(), GetDoubleVariable, SetVariable);
    for (int i = 0; i < global_string_vec.size(); ++i)
        Nan::SetAccessor(proto, Nan::New(global_string_vec[i]->name_str()).ToLocalChecked(), GetStringVariable, SetVariable);
    for (int i = 0; i < member_string_vec.size(); ++i)
        Nan::SetAccessor(proto, Nan::New(member_string_vec[i]->name_str()).ToLocalChecked(), GetStringVariable, SetVariable);

    Nan::SetPrototypeMethod(constructor_template, "clear", Clear);
    Nan::SetPrototypeMethod(constructor_template, "clearAdaptiveClassifier", ClearAdaptiveClassifier);
    Nan::SetPrototypeMethod(constructor_template, "thresholdImage", ThresholdImage);
    Nan::SetPrototypeMethod(constructor_template, "findRegions", FindRegions);
    Nan::SetPrototypeMethod(constructor_template, "findParagraphs", FindParagraphs);
    Nan::SetPrototypeMethod(constructor_template, "findTextLines", FindTextLines);
    Nan::SetPrototypeMethod(constructor_template, "findWords", FindWords);
    Nan::SetPrototypeMethod(constructor_template, "findSymbols", FindSymbols);
    Nan::SetPrototypeMethod(constructor_template, "findText", FindText);
    
    target->Set(Nan::New("Tesseract").ToLocalChecked(), constructor_template->GetFunction());
}

NAN_METHOD(Tesseract::New)
{
    Local<String> datapath;
    Local<String> lang;
    Local<Object> image;
	if (info.Length() == 1 && info[0]->IsString()) {
        datapath = info[0]->ToString();
        lang = Nan::New<String>("eng").ToLocalChecked();
    } else if (info.Length() == 2 && info[0]->IsString() && info[1]->IsString()) {
        datapath = info[0]->ToString();
        lang = info[1]->ToString();
    } else if (info.Length() == 3 && info[0]->IsString() && info[1]->IsString()
               && Image::HasInstance(info[2])) {
        datapath = info[0]->ToString();
        lang = info[1]->ToString();
        image = info[2]->ToObject();
    } else {
        return Nan::ThrowTypeError("cannot convert argument list to "
                     "(datapath: String) or "
                     "(datapath: String, language: String) or "
                     "(datapath: String, language: String, image: Image)");
    }
    Tesseract* obj = new Tesseract(*String::Utf8Value(datapath),
                                   *String::Utf8Value(lang));
    if (!image.IsEmpty()) {
        Local<Object> image_ = image->ToObject();
        obj->image_.Reset(image_);
        obj->api_.SetImage(Image::Pixels(image_));
    }
    obj->Wrap(info.This());
}

NAN_GETTER(Tesseract::GetImage)
{
    Nan::HandleScope scope;
    Tesseract* obj = Nan::ObjectWrap::Unwrap<Tesseract>(info.This());
    info.GetReturnValue().Set(Nan::New(obj->image_));
}

NAN_SETTER(Tesseract::SetImage)
{
    Nan::HandleScope scope;
    Tesseract* obj = Nan::ObjectWrap::Unwrap<Tesseract>(info.This());
    if (Image::HasInstance(value) || value->IsNull()) {
        if (!obj->image_.IsEmpty()) {
            obj->image_.Reset();
        }
        if (!value->IsNull()) {
            Local<Object> image_ = value->ToObject();
            obj->image_.Reset(image_);
            obj->api_.SetImage(Image::Pixels(image_));
        } else {
            obj->api_.Clear();
        }
    } else {
        Nan::ThrowTypeError("value must be of type Image");
    }
}

NAN_GETTER(Tesseract::GetRectangle)
{
    Nan::HandleScope scope;
    Tesseract* obj = Nan::ObjectWrap::Unwrap<Tesseract>(info.This());
    info.GetReturnValue().Set(Nan::New(obj->rectangle_));
}

NAN_SETTER(Tesseract::SetRectangle)
{
    Nan::HandleScope scope;
    Tesseract* obj = Nan::ObjectWrap::Unwrap<Tesseract>(info.This());
    Local<Object> rect = value->ToObject();
    if (value->IsObject()) {
        if (!obj->rectangle_.IsEmpty()) {
            obj->rectangle_.Reset();
        }
        obj->rectangle_.Reset(rect);
        int x = floor(rect->Get(Nan::New("x").ToLocalChecked())->ToNumber()->Value());
        int y = floor(rect->Get(Nan::New("y").ToLocalChecked())->ToNumber()->Value());
        int width = ceil(rect->Get(Nan::New("width").ToLocalChecked())->ToNumber()->Value());
        int height = ceil(rect->Get(Nan::New("height").ToLocalChecked())->ToNumber()->Value());
        if (!obj->image_.IsEmpty()) {
            // WORKAROUND: clamp rectangle to prevent occasional crashes.
            PIX* pix = Image::Pixels(Nan::New<Object>(obj->image_));
            x = (std::max)(x, 0);
            y = (std::max)(y, 0);
            width = (std::min)(width, (int)pix->w - x);
            height = (std::min)(height, (int)pix->h - y);
        }
        obj->api_.SetRectangle(x, y, width, height);
    } else {
        Nan::ThrowTypeError("value must be of type Object with at least "
              "x, y, width and height properties");
    }
}

NAN_GETTER(Tesseract::GetPageSegMode)
{
    Nan::HandleScope scope;
    Tesseract* obj = Nan::ObjectWrap::Unwrap<Tesseract>(info.This());
    switch (obj->api_.GetPageSegMode()) {
    case tesseract::PSM_OSD_ONLY:
        ReturnValue("osd_only");
    case tesseract::PSM_AUTO_OSD:
        ReturnValue("auto_osd");
    case tesseract::PSM_AUTO_ONLY:
        ReturnValue("auto_only");
    case tesseract::PSM_AUTO:
        ReturnValue("auto");
    case tesseract::PSM_SINGLE_COLUMN:
        ReturnValue("single_column");
    case tesseract::PSM_SINGLE_BLOCK_VERT_TEXT:
        ReturnValue("single_block_vert_text");
    case tesseract::PSM_SINGLE_BLOCK:
        ReturnValue("single_block");
    case tesseract::PSM_SINGLE_LINE:
        ReturnValue("single_line");
    case tesseract::PSM_SINGLE_WORD:
        ReturnValue("single_word");
    case tesseract::PSM_CIRCLE_WORD:
        ReturnValue("circle_word");
    case tesseract::PSM_SINGLE_CHAR:
        ReturnValue("single_char");
    case tesseract::PSM_SPARSE_TEXT:
        ReturnValue("sparse_text");
    case tesseract::PSM_SPARSE_TEXT_OSD:
        ReturnValue("sparse_text_osd");
    default:
        return Nan::ThrowError("cannot convert internal PSM to String");
    }
}

NAN_SETTER(Tesseract::SetPageSegMode)
{
    Nan::HandleScope scope;
    Tesseract* obj = Nan::ObjectWrap::Unwrap<Tesseract>(info.This());
    String::Utf8Value pageSegMode(value);
    if (strcmp("osd_only", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_OSD_ONLY);
    } else if (strcmp("auto_osd", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_AUTO_OSD);
    } else if (strcmp("auto_only", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_AUTO_ONLY);
    } else if (strcmp("auto", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_AUTO);
    } else if (strcmp("single_column", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_SINGLE_COLUMN);
    } else if (strcmp("single_block_vert_text", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_SINGLE_BLOCK_VERT_TEXT);
    } else if (strcmp("single_block", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
    } else if (strcmp("single_line", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_SINGLE_LINE);
    } else if (strcmp("single_word", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_SINGLE_WORD);
    } else if (strcmp("circle_word", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_CIRCLE_WORD);
    } else if (strcmp("single_char", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_SINGLE_CHAR);
    } else if (strcmp("sparse_text", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_SPARSE_TEXT);
    } else if (strcmp("sparse_text_osd", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_SPARSE_TEXT_OSD);
    } else {
        Nan::ThrowTypeError("value must be of type String. "
              "Valid values are: "
              "osd_only, auto_osd, auto_only, auto, single_column, "
              "single_block_vert_text, single_block, single_line, "
              "single_word, circle_word, single_char, sparse_text, "
              "sparse_text_osd");
    }
}

NAN_GETTER(Tesseract::GetSymbolWhitelist)
{
    Nan::HandleScope scope;
    Tesseract* obj = Nan::ObjectWrap::Unwrap<Tesseract>(info.This());
    ReturnValue(obj->api_.GetStringVariable("tessedit_char_whitelist"));
}

NAN_SETTER(Tesseract::SetSymbolWhitelist)
{
    Nan::HandleScope scope;
    Tesseract* obj = Nan::ObjectWrap::Unwrap<Tesseract>(info.This());
    if (value->IsString()) {
        String::Utf8Value whitelist(value);
        obj->api_.SetVariable("tessedit_char_whitelist", *whitelist);
    } else {
        Nan::ThrowTypeError("value must be of type string");
    }
}

NAN_SETTER(Tesseract::SetVariable)
{
    Nan::HandleScope scope;
    Tesseract* obj = Nan::ObjectWrap::Unwrap<Tesseract>(info.This());
    String::Utf8Value name(property);
    String::Utf8Value val(value);
    obj->api_.SetVariable(*name, *val);
}

NAN_GETTER(Tesseract::GetIntVariable)
{
    Nan::HandleScope scope;
    Tesseract* obj = Nan::ObjectWrap::Unwrap<Tesseract>(info.This());
    String::Utf8Value name(property);
    int value;
    if(obj->api_.GetIntVariable(*name, &value)) {
        info.GetReturnValue().Set(Nan::New(value));
    }
    else {
    	info.GetReturnValue().SetNull();
    }
}

NAN_GETTER(Tesseract::GetBoolVariable)
{
    Nan::HandleScope scope;
    Tesseract* obj = Nan::ObjectWrap::Unwrap<Tesseract>(info.This());
    String::Utf8Value name(property);
    bool value;
    if (obj->api_.GetBoolVariable(*name, &value)) {
        info.GetReturnValue().Set(Nan::New(value));
    } else {
        info.GetReturnValue().SetNull();
    }
}

NAN_GETTER(Tesseract::GetDoubleVariable)
{
    Nan::HandleScope scope;
    Tesseract* obj = Nan::ObjectWrap::Unwrap<Tesseract>(info.This());
    String::Utf8Value name(property);
    double value;
    if(obj->api_.GetDoubleVariable(*name, &value)) {
        info.GetReturnValue().Set(Nan::New(value));
    }
    else {
        info.GetReturnValue().SetNull();
    }
}

NAN_GETTER(Tesseract::GetStringVariable)
{
    Nan::HandleScope scope;
    Tesseract* obj = Nan::ObjectWrap::Unwrap<Tesseract>(info.This());
    String::Utf8Value name(property);
    const char *p = obj->api_.GetStringVariable(*name);
    if((p != NULL)) {
        ReturnValue(p);
    }
    else {
        info.GetReturnValue().SetNull();
    }
}

NAN_METHOD(Tesseract::Clear)
{
	Tesseract* obj = Nan::ObjectWrap::Unwrap<Tesseract>(info.This());
    obj->api_.Clear();
    info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Tesseract::ClearAdaptiveClassifier)
{
    Tesseract* obj = Nan::ObjectWrap::Unwrap<Tesseract>(info.This());
    obj->api_.ClearAdaptiveClassifier();
    info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Tesseract::ThresholdImage)
{
    Tesseract* obj = Nan::ObjectWrap::Unwrap<Tesseract>(info.This());
    Pix *pix = obj->api_.GetThresholdedImage();
    if (pix) {
        info.GetReturnValue().Set(Image::New(pix));
    } else {
        info.GetReturnValue().SetNull();
    }
}

NAN_METHOD(Tesseract::FindRegions)
{
    Tesseract* obj = Nan::ObjectWrap::Unwrap<Tesseract>(info.This());
    return obj->TransformResult(tesseract::RIL_BLOCK, info);
}

NAN_METHOD(Tesseract::FindParagraphs)
{
    Tesseract* obj = Nan::ObjectWrap::Unwrap<Tesseract>(info.This());
    return obj->TransformResult(tesseract::RIL_PARA, info);
}

NAN_METHOD(Tesseract::FindTextLines)
{
    Tesseract* obj = Nan::ObjectWrap::Unwrap<Tesseract>(info.This());
    return obj->TransformResult(tesseract::RIL_TEXTLINE, info);
}

NAN_METHOD(Tesseract::FindWords)
{
    Tesseract* obj = Nan::ObjectWrap::Unwrap<Tesseract>(info.This());
    return obj->TransformResult(tesseract::RIL_WORD, info);
}

NAN_METHOD(Tesseract::FindSymbols)
{
    Tesseract* obj = Nan::ObjectWrap::Unwrap<Tesseract>(info.This());
    return obj->TransformResult(tesseract::RIL_SYMBOL, info);
}

NAN_METHOD(Tesseract::FindText)
{
    Tesseract* obj = Nan::ObjectWrap::Unwrap<Tesseract>(info.This());
    if (info.Length() >= 1 && info[0]->IsString()) {
        String::Utf8Value mode(info[0]);
        bool withConfidence = false;
        if (info.Length() == 2 && info[1]->IsBoolean()) {
            withConfidence = info[1]->BooleanValue();
        } else if (info.Length() == 3 && info[2]->IsBoolean()) {
            withConfidence = info[2]->BooleanValue();
        }
        const char *text = NULL;
        bool modeValid = true;
        if (strcmp("plain", *mode) == 0) {
            text = obj->api_.GetUTF8Text();
        } else if (strcmp("unlv", *mode) == 0) {
            text = obj->api_.GetUNLVText();
        } else if (strcmp("hocr", *mode) == 0 && info.Length() == 2 && info[1]->IsInt32()) {
            text = obj->api_.GetHOCRText(info[1]->Int32Value());
        } else if (strcmp("box", *mode) == 0 && info.Length() == 2 && info[1]->IsInt32()) {
            text = obj->api_.GetBoxText(info[1]->Int32Value());
        } else {
            modeValid = false;
        }
        if (modeValid) {
            if (!text) {
                return Nan::ThrowError("Internal tesseract error");
            } else if (withConfidence) {
                Local<Object> result = Nan::New<Object>();
                result->Set(Nan::New("text").ToLocalChecked(), Nan::New<String>(text).ToLocalChecked());
                // Don't "delete[] text;": it breaks Tesseract 3.02 (documentation bug?)
                result->Set(Nan::New("confidence").ToLocalChecked(), Nan::New<Number>(obj->api_.MeanTextConf()));
                info.GetReturnValue().Set(result);
                return;
            } else {
                ReturnValue(text);
            }
        }
    }
    return Nan::ThrowTypeError("cannot convert argument list to "
                 "(\"plain\", [withConfidence]) or "
                 "(\"unlv\", [withConfidence]) or "
                 "(\"hocr\", pageNumber: Int32, [withConfidence]) or "
                 "(\"box\", pageNumber: Int32, [withConfidence])");
}

Tesseract::Tesseract(const char *datapath, const char *language)
{
    int res = api_.Init(datapath, language, tesseract::OEM_DEFAULT);
    api_.SetVariable("save_blob_choices", "T");
    assert(res == 0);
}

Tesseract::~Tesseract()
{
    api_.End();
}

Nan::NAN_METHOD_RETURN_TYPE Tesseract::TransformResult(tesseract::PageIteratorLevel level, Nan::NAN_METHOD_ARGS_TYPE args)
{
    Nan::HandleScope scope;
    bool recognize = true;
    if (args.Length() >= 1 && args[0]->IsBoolean()) {
        recognize = args[0]->BooleanValue();
    }
    tesseract::PageIterator *it = 0;
    if (recognize) {
        if (api_.Recognize(NULL) != 0) {
            return Nan::ThrowError("Internal tesseract error");
        }
        it = api_.GetIterator();
    } else {
        it = api_.AnalyseLayout();
    }
    Local<Array> results = Nan::New<Array>();
    if (it == NULL) {
        args.GetReturnValue().Set(results);
        return;
    }
    int index = 0;
    do {
        if (it->Empty(level)) {
            continue;
        }
        Local<Object> result = Nan::New<Object>();
        int left, top, right, bottom;
        if (it->BoundingBoxInternal(level, &left, &top, &right, &bottom)) {
            // Extract image coordiante box.
            Handle<Object> box = Nan::New<Object>();
            box->Set(Nan::New("x").ToLocalChecked(), Nan::New<Int32>(left));
            box->Set(Nan::New("y").ToLocalChecked(), Nan::New<Int32>(top));
            box->Set(Nan::New("width").ToLocalChecked(), Nan::New<Int32>(right - left));
            box->Set(Nan::New("height").ToLocalChecked(), Nan::New<Int32>(bottom - top));
            result->Set(Nan::New("box").ToLocalChecked(), box);
        }
        if (level != tesseract::RIL_TEXTLINE && recognize) {
            // Extract text.
            char *text = static_cast<tesseract::ResultIterator *>(it)->GetUTF8Text(level);
            if (text) {
                result->Set(Nan::New("text").ToLocalChecked(), Nan::New(text).ToLocalChecked());
                delete[] text;
                // Extract confidence.
                float confidence = static_cast<tesseract::ResultIterator *>(it)->Confidence(level);
                result->Set(Nan::New("confidence").ToLocalChecked(), Nan::New<Number>(confidence));
            }
        }
        if (level == tesseract::RIL_SYMBOL && recognize) {
            // Extract choices
            tesseract::ChoiceIterator choiceIt = tesseract::ChoiceIterator(
                        *static_cast<tesseract::ResultIterator *>(it));
            Local<Array> choices = Nan::New<Array>();
            int choiceIndex = 0;
            do {
                const char* text = choiceIt.GetUTF8Text();
                if (!text) {
                    break;
                }
                // Transform choice to object.
                Local<Object> choice = Nan::New<Object>();
                choice->Set(Nan::New("text").ToLocalChecked(),
                            Nan::New<String>(text).ToLocalChecked());
                choice->Set(Nan::New("confidence").ToLocalChecked(),
                            Nan::New<Number>(choiceIt.Confidence()));
                // Append choice to choices list and cleanup.
                choices->Set(choiceIndex++, choice);
                // Don't "delete[] text;": it breaks Tesseract 3.02 (documentation bug?)
            } while (choiceIt.Next());
            result->Set(Nan::New("choices").ToLocalChecked(), choices);
        }
        // Append result.
        results->Set(index++, result);
    } while (it->Next(level));
    delete it;
    args.GetReturnValue().Set(results);
}

}
