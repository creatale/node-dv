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
#include "tesseract.h"
#include "image.h"
#include "util.h"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <strngs.h>
#include <resultiterator.h>

using namespace v8;
using namespace node;

void Tesseract::Init(Handle<Object> target)
{
    Local<FunctionTemplate> constructor_template = FunctionTemplate::New(New);
    constructor_template->SetClassName(String::NewSymbol("Tesseract"));
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    Local<ObjectTemplate> proto = constructor_template->PrototypeTemplate();
    proto->SetAccessor(String::NewSymbol("image"), GetImage, SetImage);
    proto->SetAccessor(String::NewSymbol("rectangle"), GetRectangle, SetRectangle);
    proto->SetAccessor(String::NewSymbol("pageSegMode"), GetPageSegMode, SetPageSegMode);
    proto->SetAccessor(String::NewSymbol("symbolWhitelist"), GetSymbolWhitelist, SetSymbolWhitelist);
    proto->Set(String::NewSymbol("clear"),
               FunctionTemplate::New(Clear)->GetFunction());
    proto->Set(String::NewSymbol("clearAdaptiveClassifier"),
               FunctionTemplate::New(ClearAdaptiveClassifier)->GetFunction());
    proto->Set(String::NewSymbol("thresholdImage"),
               FunctionTemplate::New(ThresholdImage)->GetFunction());
    proto->Set(String::NewSymbol("findRegions"),
               FunctionTemplate::New(FindRegions)->GetFunction());
    proto->Set(String::NewSymbol("findParagraphs"),
               FunctionTemplate::New(FindParagraphs)->GetFunction());
    proto->Set(String::NewSymbol("findTextLines"),
               FunctionTemplate::New(FindTextLines)->GetFunction());
    proto->Set(String::NewSymbol("findWords"),
               FunctionTemplate::New(FindWords)->GetFunction());
    proto->Set(String::NewSymbol("findSymbols"),
               FunctionTemplate::New(FindSymbols)->GetFunction());
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
        return THROW(TypeError, "cannot convert argument list to "
                     "(datapath: String) or "
                     "(datapath: String, language: String) or "
                     "(datapath: String, language: String, image: Image)");
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

Handle<Value> Tesseract::GetImage(Local<String> prop, const AccessorInfo &info)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(info.This());
    return scope.Close(obj->image_);
}

void Tesseract::SetImage(Local<String> prop, Local<Value> value, const AccessorInfo &info)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(info.This());
    if (Image::HasInstance(value)) {
        if (!obj->image_.IsEmpty()) {
            obj->image_.Dispose();
            obj->image_.Clear();
        }
        obj->image_ = Persistent<Object>::New(value->ToObject());
        obj->api_.SetImage(Image::Pixels(obj->image_));
    } else {
        THROW(TypeError, "value must be of type Image");
    }
}

Handle<Value> Tesseract::GetRectangle(Local<String> prop, const AccessorInfo &info)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(info.This());
    return scope.Close(obj->rectangle_);
}

void Tesseract::SetRectangle(Local<String> prop, Local<Value> value, const AccessorInfo &info)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(info.This());
    Local<Object> rect = value->ToObject();
    if (value->IsObject()) {
        if (!obj->rectangle_.IsEmpty()) {
            obj->rectangle_.Dispose();
            obj->rectangle_.Clear();
        }
        obj->rectangle_ = Persistent<Object>::New(rect);
        int x = floor(rect->Get(String::NewSymbol("x"))->ToNumber()->Value());
        int y = floor(rect->Get(String::NewSymbol("y"))->ToNumber()->Value());
        int width = ceil(rect->Get(String::NewSymbol("width"))->ToNumber()->Value());
        int height = ceil(rect->Get(String::NewSymbol("height"))->ToNumber()->Value());
        if (!obj->image_.IsEmpty()) {
            // WORKAROUND: clamp rectangle to prevent occasional crashes.
            PIX* pix = Image::Pixels(obj->image_);
            x = std::max(x, 0);
            y = std::max(y, 0);
            width = std::min(width, (int)pix->w - x);
            height = std::min(height, (int)pix->h - y);
        }
        obj->api_.SetRectangle(x, y, width, height);
    } else {
        THROW(TypeError, "value must be of type Object with at least "
              "x, y, width and height properties");
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
    case tesseract::PSM_SPARSE_TEXT:
        return String::New("sparse_text");
    case tesseract::PSM_SPARSE_TEXT_OSD:
        return String::New("sparse_text_osd");
    default:
        return THROW(Error, "cannot convert internal PSM to String");
    }
}

void Tesseract::SetPageSegMode(Local<String> prop, Local<Value> value, const AccessorInfo &info)
{
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(info.This());
    String::AsciiValue pageSegMode(value);
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
        THROW(TypeError, "value must be of type String. "
              "Valid values are: "
              "osd_only, auto_osd, auto_only, auto, single_column, "
              "single_block_vert_text, single_block, single_line, "
              "single_word, circle_word, single_char, sparse_text, "
              "sparse_text_osd");
    }
}

Handle<Value> Tesseract::GetSymbolWhitelist(Local<String> prop, const AccessorInfo &info)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(info.This());
    return scope.Close(String::New(obj->api_.GetStringVariable("tessedit_char_whitelist")));
}

void Tesseract::SetSymbolWhitelist(Local<String> prop, Local<Value> value, const AccessorInfo &info)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(info.This());
    if (value->IsString()) {
        String::AsciiValue whitelist(value);
        obj->api_.SetVariable("tessedit_char_whitelist", *whitelist);
    } else {
        THROW(TypeError, "value must be of type string");
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
    if (pix) {
        return scope.Close(Image::New(pix));
    } else {
        return scope.Close(Null());
    }
}

Handle<Value> Tesseract::FindRegions(const Arguments &args)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    return scope.Close(obj->TransformResult(tesseract::RIL_BLOCK, args));
}

Handle<Value> Tesseract::FindParagraphs(const Arguments &args)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    return scope.Close(obj->TransformResult(tesseract::RIL_PARA, args));
}

Handle<Value> Tesseract::FindTextLines(const Arguments &args)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    return scope.Close(obj->TransformResult(tesseract::RIL_TEXTLINE, args));
}

Handle<Value> Tesseract::FindWords(const Arguments &args)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    return scope.Close(obj->TransformResult(tesseract::RIL_WORD, args));
}

Handle<Value> Tesseract::FindSymbols(const Arguments &args)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    return scope.Close(obj->TransformResult(tesseract::RIL_SYMBOL, args));
}

Handle<Value> Tesseract::FindText(const Arguments &args)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    if (args.Length() >= 1 && args[0]->IsString()) {
        String::AsciiValue mode(args[0]);
        bool withConfidence = false;
        if (args.Length() == 2 && args[1]->IsBoolean()) {
            withConfidence = args[1]->BooleanValue();
        } else if (args.Length() == 3 && args[2]->IsBoolean()) {
            withConfidence = args[2]->BooleanValue();
        }
        const char *text = NULL;
        if (strcmp("plain", *mode) == 0) {
            text = obj->api_.GetUTF8Text();
        } else if (strcmp("unlv", *mode) == 0) {
            text = obj->api_.GetUNLVText();
        } else if (strcmp("hocr", *mode) == 0 && args.Length() == 2 && args[1]->IsInt32()) {
            text = obj->api_.GetHOCRText(args[1]->Int32Value());
        } else if (strcmp("box", *mode) == 0 && args.Length() == 2 && args[1]->IsInt32()) {
            text = obj->api_.GetBoxText(args[1]->Int32Value());
        }
        if (!text) {
            return THROW(Error, "Internal tesseract error");
        }
        if (withConfidence) {
            Handle<Object> result = Object::New();
            result->Set(String::NewSymbol("text"), String::New(text));
            // Don't "delete[] text;": it breaks Tesseract 3.02 (documentation bug?)
            result->Set(String::NewSymbol("confidence"), Number::New(obj->api_.MeanTextConf()));
            return scope.Close(result);
        } else {
            Local<String> textString = String::New(text);
            // Don't "delete[] text;": it breaks Tesseract 3.02 (documentation bug?)
            return scope.Close(textString);
        }
    }
    return THROW(TypeError, "cannot convert argument list to "
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

Handle<Value> Tesseract::TransformResult(tesseract::PageIteratorLevel level, const Arguments &args)
{
    bool recognize = true;
    if (args.Length() >= 1 && args[0]->IsBoolean()) {
        recognize = args[0]->BooleanValue();
    }
    tesseract::PageIterator *it = 0;
    if (recognize) {
        if (api_.Recognize(NULL) != 0) {
            return THROW(Error, "Internal tesseract error");
        }
        it = api_.GetIterator();
    } else {
        it = api_.AnalyseLayout();
    }
    Local<Array> results = Array::New();
    if (it == NULL) {
        return results;
    }
    int index = 0;
    do {
        if (it->Empty(level)) {
            continue;
        }
        Handle<Object> result = Object::New();
        int left, top, right, bottom;
        if (it->BoundingBoxInternal(level, &left, &top, &right, &bottom)) {
            // Extract image coordiante box.
            Handle<Object> box = Object::New();
            box->Set(String::NewSymbol("x"), Int32::New(left));
            box->Set(String::NewSymbol("y"), Int32::New(top));
            box->Set(String::NewSymbol("width"), Int32::New(right - left));
            box->Set(String::NewSymbol("height"), Int32::New(bottom - top));
            result->Set(String::NewSymbol("box"), box);
        }
        if (level != tesseract::RIL_TEXTLINE && recognize) {
            // Extract text.
            char *text = static_cast<tesseract::ResultIterator *>(it)->GetUTF8Text(level);
            if (text) {
                result->Set(String::NewSymbol("text"), String::New(text));
                delete[] text;
                // Extract confidence.
                float confidence = static_cast<tesseract::ResultIterator *>(it)->Confidence(level);
                result->Set(String::NewSymbol("confidence"), Number::New(confidence));
            }
        }
        if (level == tesseract::RIL_SYMBOL && recognize) {
            // Extract choices
            tesseract::ChoiceIterator choiceIt = tesseract::ChoiceIterator(
                        *static_cast<tesseract::ResultIterator *>(it));
            Handle<Array> choices = Array::New();
            int choiceIndex = 0;
            do {
                const char* text = choiceIt.GetUTF8Text();
                if (!text) {
                    break;
                }
                // Transform choice to object.
                Local<Object> choice = Object::New();
                choice->Set(String::NewSymbol("text"),
                            String::New(text));
                choice->Set(String::NewSymbol("confidence"),
                            Number::New(choiceIt.Confidence()));
                // Append choice to choices list and cleanup.
                choices->Set(choiceIndex++, choice);
                // Don't "delete[] text;": it breaks Tesseract 3.02 (documentation bug?)
            } while (choiceIt.Next());
            result->Set(String::NewSymbol("choices"), choices);
        }
        // Append result.
        results->Set(index++, result);
    } while (it->Next(level));
    delete it;
    return results;
}
