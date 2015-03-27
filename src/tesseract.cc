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
using namespace node;

namespace binding {

void Tesseract::Init(Handle<Object> target)
{
    Local<FunctionTemplate> constructor_template = NanNew<FunctionTemplate>(New);
    constructor_template->SetClassName(NanNew("Tesseract"));
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    Local<ObjectTemplate> proto = constructor_template->PrototypeTemplate();
    proto->SetAccessor(NanNew("image"), GetImage, SetImage);
    proto->SetAccessor(NanNew("rectangle"), GetRectangle, SetRectangle);
    proto->SetAccessor(NanNew("pageSegMode"), GetPageSegMode, SetPageSegMode);
    proto->SetAccessor(NanNew("symbolWhitelist"), GetSymbolWhitelist, SetSymbolWhitelist); //TODO: remove (deprecated).
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
        proto->SetAccessor(NanNew(global_int_vec[i]->name_str()), GetIntVariable, SetVariable);
    for (int i = 0; i < member_int_vec.size(); ++i)
        proto->SetAccessor(NanNew(member_int_vec[i]->name_str()), GetIntVariable, SetVariable);
    for (int i = 0; i < global_bool_vec.size(); ++i)
        proto->SetAccessor(NanNew(global_bool_vec[i]->name_str()), GetBoolVariable, SetVariable);
    for (int i = 0; i < member_bool_vec.size(); ++i)
        proto->SetAccessor(NanNew(member_bool_vec[i]->name_str()), GetBoolVariable, SetVariable);
    for (int i = 0; i < global_double_vec.size(); ++i)
        proto->SetAccessor(NanNew(global_double_vec[i]->name_str()), GetDoubleVariable, SetVariable);
    for (int i = 0; i < member_double_vec.size(); ++i)
        proto->SetAccessor(NanNew(member_double_vec[i]->name_str()), GetDoubleVariable, SetVariable);
    for (int i = 0; i < global_string_vec.size(); ++i)
        proto->SetAccessor(NanNew(global_string_vec[i]->name_str()), GetStringVariable, SetVariable);
    for (int i = 0; i < member_string_vec.size(); ++i)
        proto->SetAccessor(NanNew(member_string_vec[i]->name_str()), GetStringVariable, SetVariable);
    proto->Set(NanNew("clear"),
               NanNew<FunctionTemplate>(Clear)->GetFunction());
    proto->Set(NanNew("clearAdaptiveClassifier"),
               NanNew<FunctionTemplate>(ClearAdaptiveClassifier)->GetFunction());
    proto->Set(NanNew("thresholdImage"),
               NanNew<FunctionTemplate>(ThresholdImage)->GetFunction());
    proto->Set(NanNew("findRegions"),
               NanNew<FunctionTemplate>(FindRegions)->GetFunction());
    proto->Set(NanNew("findParagraphs"),
               NanNew<FunctionTemplate>(FindParagraphs)->GetFunction());
    proto->Set(NanNew("findTextLines"),
               NanNew<FunctionTemplate>(FindTextLines)->GetFunction());
    proto->Set(NanNew("findWords"),
               NanNew<FunctionTemplate>(FindWords)->GetFunction());
    proto->Set(NanNew("findSymbols"),
               NanNew<FunctionTemplate>(FindSymbols)->GetFunction());
    proto->Set(NanNew("findText"),
               NanNew<FunctionTemplate>(FindText)->GetFunction());
    target->Set(NanNew("Tesseract"), constructor_template->GetFunction());
}

NAN_METHOD(Tesseract::New)
{
    NanScope();
    Local<String> datapath;
    Local<String> lang;
    Local<Object> image;
    if (args.Length() == 1 && args[0]->IsString()) {
        datapath = args[0]->ToString();
        lang = NanNew<String>("eng");
    } else if (args.Length() == 2 && args[0]->IsString() && args[1]->IsString()) {
        datapath = args[0]->ToString();
        lang = args[1]->ToString();
    } else if (args.Length() == 3 && args[0]->IsString() && args[1]->IsString()
               && Image::HasInstance(args[2])) {
        datapath = args[0]->ToString();
        lang = args[1]->ToString();
        image = args[2]->ToObject();
    } else {
        return NanThrowTypeError("cannot convert argument list to "
                     "(datapath: String) or "
                     "(datapath: String, language: String) or "
                     "(datapath: String, language: String, image: Image)");
    }
    Tesseract* obj = new Tesseract(*String::Utf8Value(datapath),
                                   *String::Utf8Value(lang));
    if (!image.IsEmpty()) {
        Local<Object> image_ = image->ToObject();
        NanAssignPersistent(obj->image_, image_);
        obj->api_.SetImage(Image::Pixels(image_));
    }
    obj->Wrap(args.This());
    NanReturnThis();
}

NAN_GETTER(Tesseract::GetImage)
{
    NanScope();
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    NanReturnValue(obj->image_);
}

NAN_SETTER(Tesseract::SetImage)
{
    NanScope();
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    if (Image::HasInstance(value) || value->IsNull()) {
        if (!obj->image_.IsEmpty()) {
            NanDisposePersistent(obj->image_);
        }
        if (!value->IsNull()) {
            Local<Object> image_ = value->ToObject();
            NanAssignPersistent(obj->image_, image_);
            obj->api_.SetImage(Image::Pixels(image_));
        } else {
            obj->api_.Clear();
        }
    } else {
        return NanThrowTypeError("value must be of type Image");
    }
}

NAN_GETTER(Tesseract::GetRectangle)
{
    NanScope();
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    NanReturnValue(obj->rectangle_);
}

NAN_SETTER(Tesseract::SetRectangle)
{
    NanScope();
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    Local<Object> rect = value->ToObject();
    if (value->IsObject()) {
        if (!obj->rectangle_.IsEmpty()) {
            NanDisposePersistent(obj->rectangle_);
        }
        NanAssignPersistent(obj->rectangle_, rect);
        int x = floor(rect->Get(NanNew("x"))->ToNumber()->Value());
        int y = floor(rect->Get(NanNew("y"))->ToNumber()->Value());
        int width = ceil(rect->Get(NanNew("width"))->ToNumber()->Value());
        int height = ceil(rect->Get(NanNew("height"))->ToNumber()->Value());
        if (!obj->image_.IsEmpty()) {
            // WORKAROUND: clamp rectangle to prevent occasional crashes.
            PIX* pix = Image::Pixels(NanNew<Object>(obj->image_));
            x = std::max(x, 0);
            y = std::max(y, 0);
            width = std::min(width, (int)pix->w - x);
            height = std::min(height, (int)pix->h - y);
        }
        obj->api_.SetRectangle(x, y, width, height);
    } else {
        NanThrowTypeError("value must be of type Object with at least "
              "x, y, width and height properties");
    }
}

NAN_GETTER(Tesseract::GetPageSegMode)
{
    NanScope();
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    switch (obj->api_.GetPageSegMode()) {
    case tesseract::PSM_OSD_ONLY:
        NanReturnValue(NanNew<String>("osd_only"));
    case tesseract::PSM_AUTO_OSD:
        NanReturnValue(NanNew<String>("auto_osd"));
    case tesseract::PSM_AUTO_ONLY:
        NanReturnValue(NanNew<String>("auto_only"));
    case tesseract::PSM_AUTO:
        NanReturnValue(NanNew<String>("auto"));
    case tesseract::PSM_SINGLE_COLUMN:
        NanReturnValue(NanNew<String>("single_column"));
    case tesseract::PSM_SINGLE_BLOCK_VERT_TEXT:
        NanReturnValue(NanNew<String>("single_block_vert_text"));
    case tesseract::PSM_SINGLE_BLOCK:
        NanReturnValue(NanNew<String>("single_block"));
    case tesseract::PSM_SINGLE_LINE:
        NanReturnValue(NanNew<String>("single_line"));
    case tesseract::PSM_SINGLE_WORD:
        NanReturnValue(NanNew<String>("single_word"));
    case tesseract::PSM_CIRCLE_WORD:
        NanReturnValue(NanNew<String>("circle_word"));
    case tesseract::PSM_SINGLE_CHAR:
        NanReturnValue(NanNew<String>("single_char"));
    case tesseract::PSM_SPARSE_TEXT:
        NanReturnValue(NanNew<String>("sparse_text"));
    case tesseract::PSM_SPARSE_TEXT_OSD:
        NanReturnValue(NanNew<String>("sparse_text_osd"));
    default:
        return NanThrowError("cannot convert internal PSM to String");
    }
}

NAN_SETTER(Tesseract::SetPageSegMode)
{
    NanScope();
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
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
        NanThrowTypeError("value must be of type String. "
              "Valid values are: "
              "osd_only, auto_osd, auto_only, auto, single_column, "
              "single_block_vert_text, single_block, single_line, "
              "single_word, circle_word, single_char, sparse_text, "
              "sparse_text_osd");
    }
}

NAN_GETTER(Tesseract::GetSymbolWhitelist)
{
    NanScope();
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    NanReturnValue(NanNew<String>(obj->api_.GetStringVariable("tessedit_char_whitelist")));
}

NAN_SETTER(Tesseract::SetSymbolWhitelist)
{
    NanScope();
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    if (value->IsString()) {
        String::Utf8Value whitelist(value);
        obj->api_.SetVariable("tessedit_char_whitelist", *whitelist);
    } else {
        return NanThrowTypeError("value must be of type string");
    }
}

NAN_SETTER(Tesseract::SetVariable)
{
    NanScope();
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    String::Utf8Value name(property);
    String::Utf8Value val(value);
    obj->api_.SetVariable(*name, *val);
}

NAN_GETTER(Tesseract::GetIntVariable)
{
    NanScope();
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    String::Utf8Value name(property);
    int value;
    if(obj->api_.GetIntVariable(*name, &value)) {
        NanReturnValue(NanNew<Number>(value));
    }
    else {
        NanReturnNull();
    }
}

NAN_GETTER(Tesseract::GetBoolVariable)
{
    NanScope();
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    String::Utf8Value name(property);
    bool value;
    if (obj->api_.GetBoolVariable(*name, &value)) {
        NanReturnValue(NanNew<Boolean>(value));
    } else {
        NanReturnValue(NanNull());
    }
}

NAN_GETTER(Tesseract::GetDoubleVariable)
{
    NanScope();
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    String::Utf8Value name(property);
    double value;
    if(obj->api_.GetDoubleVariable(*name, &value)) {
        NanReturnValue(NanNew<Number>(value));
    }
    else {
        NanReturnNull();
    }
}

NAN_GETTER(Tesseract::GetStringVariable)
{
    NanScope();
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    String::Utf8Value name(property);
    const char *p = obj->api_.GetStringVariable(*name);
    if((p != NULL)) {
        NanReturnValue(NanNew<String>(p));
    }
    else {
        NanReturnNull();
    }
}

NAN_METHOD(Tesseract::Clear)
{
    NanScope();
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    obj->api_.Clear();
    NanReturnThis();
}

NAN_METHOD(Tesseract::ClearAdaptiveClassifier)
{
    NanScope();
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    obj->api_.ClearAdaptiveClassifier();
    NanReturnThis();
}

NAN_METHOD(Tesseract::ThresholdImage)
{
    NanScope();
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    Pix *pix = obj->api_.GetThresholdedImage();
    if (pix) {
        NanReturnValue(Image::New(pix));
    } else {
        NanReturnValue(NanNull());
    }
}

NAN_METHOD(Tesseract::FindRegions)
{
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    return obj->TransformResult(tesseract::RIL_BLOCK, args);
}

NAN_METHOD(Tesseract::FindParagraphs)
{
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    return obj->TransformResult(tesseract::RIL_PARA, args);
}

NAN_METHOD(Tesseract::FindTextLines)
{
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    return obj->TransformResult(tesseract::RIL_TEXTLINE, args);
}

NAN_METHOD(Tesseract::FindWords)
{
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    return obj->TransformResult(tesseract::RIL_WORD, args);
}

NAN_METHOD(Tesseract::FindSymbols)
{
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    return obj->TransformResult(tesseract::RIL_SYMBOL, args);
}

NAN_METHOD(Tesseract::FindText)
{
    NanScope();
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    if (args.Length() >= 1 && args[0]->IsString()) {
        String::Utf8Value mode(args[0]);
        bool withConfidence = false;
        if (args.Length() == 2 && args[1]->IsBoolean()) {
            withConfidence = args[1]->BooleanValue();
        } else if (args.Length() == 3 && args[2]->IsBoolean()) {
            withConfidence = args[2]->BooleanValue();
        }
        const char *text = NULL;
        bool modeValid = true;
        if (strcmp("plain", *mode) == 0) {
            text = obj->api_.GetUTF8Text();
        } else if (strcmp("unlv", *mode) == 0) {
            text = obj->api_.GetUNLVText();
        } else if (strcmp("hocr", *mode) == 0 && args.Length() == 2 && args[1]->IsInt32()) {
            text = obj->api_.GetHOCRText(args[1]->Int32Value());
        } else if (strcmp("box", *mode) == 0 && args.Length() == 2 && args[1]->IsInt32()) {
            text = obj->api_.GetBoxText(args[1]->Int32Value());
        } else {
            modeValid = false;
        }
        if (modeValid) {
            if (!text) {
                return NanThrowError("Internal tesseract error");
            } else if (withConfidence) {
                Handle<Object> result = NanNew<Object>();
                result->Set(NanNew("text"), NanNew<String>(text));
                // Don't "delete[] text;": it breaks Tesseract 3.02 (documentation bug?)
                result->Set(NanNew("confidence"), NanNew<Number>(obj->api_.MeanTextConf()));
                NanReturnValue(result);
            } else {
                Local<String> textString = NanNew<String>(text);
                // Don't "delete[] text;": it breaks Tesseract 3.02 (documentation bug?)
                NanReturnValue(textString);
            }
        }
    }
    return NanThrowTypeError("cannot convert argument list to "
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

_NAN_METHOD_RETURN_TYPE Tesseract::TransformResult(tesseract::PageIteratorLevel level, _NAN_METHOD_ARGS)
{
    NanScope();
    bool recognize = true;
    if (args.Length() >= 1 && args[0]->IsBoolean()) {
        recognize = args[0]->BooleanValue();
    }
    tesseract::PageIterator *it = 0;
    if (recognize) {
        if (api_.Recognize(NULL) != 0) {
            return NanThrowError("Internal tesseract error");
        }
        it = api_.GetIterator();
    } else {
        it = api_.AnalyseLayout();
    }
    Local<Array> results = NanNew<Array>();
    if (it == NULL) {
        NanReturnValue(results);
    }
    int index = 0;
    do {
        if (it->Empty(level)) {
            continue;
        }
        Handle<Object> result = NanNew<Object>();
        int left, top, right, bottom;
        if (it->BoundingBoxInternal(level, &left, &top, &right, &bottom)) {
            // Extract image coordiante box.
            Handle<Object> box = NanNew<Object>();
            box->Set(NanNew("x"), NanNew<Int32>(left));
            box->Set(NanNew("y"), NanNew<Int32>(top));
            box->Set(NanNew("width"), NanNew<Int32>(right - left));
            box->Set(NanNew("height"), NanNew<Int32>(bottom - top));
            result->Set(NanNew("box"), box);
        }
        if (level != tesseract::RIL_TEXTLINE && recognize) {
            // Extract text.
            char *text = static_cast<tesseract::ResultIterator *>(it)->GetUTF8Text(level);
            if (text) {
                result->Set(NanNew("text"), NanNew<String>(text));
                delete[] text;
                // Extract confidence.
                float confidence = static_cast<tesseract::ResultIterator *>(it)->Confidence(level);
                result->Set(NanNew("confidence"), NanNew<Number>(confidence));
            }
        }
        if (level == tesseract::RIL_SYMBOL && recognize) {
            // Extract choices
            tesseract::ChoiceIterator choiceIt = tesseract::ChoiceIterator(
                        *static_cast<tesseract::ResultIterator *>(it));
            Handle<Array> choices = NanNew<Array>();
            int choiceIndex = 0;
            do {
                const char* text = choiceIt.GetUTF8Text();
                if (!text) {
                    break;
                }
                // Transform choice to object.
                Local<Object> choice = NanNew<Object>();
                choice->Set(NanNew("text"),
                            NanNew<String>(text));
                choice->Set(NanNew("confidence"),
                            NanNew<Number>(choiceIt.Confidence()));
                // Append choice to choices list and cleanup.
                choices->Set(choiceIndex++, choice);
                // Don't "delete[] text;": it breaks Tesseract 3.02 (documentation bug?)
            } while (choiceIt.Next());
            result->Set(NanNew("choices"), choices);
        }
        // Append result.
        results->Set(index++, result);
    } while (it->Next(level));
    delete it;
    NanReturnValue(results);
}

}
