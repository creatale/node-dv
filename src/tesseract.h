/*
 * node-dv - Document Vision for node.js
 *
 * Copyright (c) 2012 Christoph Schulz
 * Copyright (c) 2013-2015 creatale GmbH, contributors listed under AUTHORS
 * 
 * MIT License <https://github.com/creatale/node-dv/blob/master/LICENSE>
 */
#ifndef TESSERACT_H
#define TESSERACT_H

#include <v8.h>
#include <node.h>
#include <baseapi.h>

namespace binding {

class Tesseract : public node::ObjectWrap
{
public:
    static void Init(v8::Handle<v8::Object> target);

private:
    static v8::Handle<v8::Value> New(const v8::Arguments& args);

    // Accessors.
    static v8::Handle<v8::Value> GetImage(v8::Local<v8::String> prop, const v8::AccessorInfo &info);
    static void SetImage(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> GetRectangle(v8::Local<v8::String> prop, const v8::AccessorInfo &info);
    static void SetRectangle(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> GetPageSegMode(v8::Local<v8::String> prop, const v8::AccessorInfo &info);
    static void SetPageSegMode(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> GetSymbolWhitelist(v8::Local<v8::String> prop, const v8::AccessorInfo &info);
    static void SetSymbolWhitelist(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::AccessorInfo &info);
    static void SetVariable(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> GetIntVariable(v8::Local<v8::String> prop, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> GetBoolVariable(v8::Local<v8::String> prop, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> GetDoubleVariable(v8::Local<v8::String> prop, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> GetStringVariable(v8::Local<v8::String> prop, const v8::AccessorInfo &info);

    // Methods.
    static v8::Handle<v8::Value> Clear(const v8::Arguments& args);
    static v8::Handle<v8::Value> ClearAdaptiveClassifier(const v8::Arguments& args);
    static v8::Handle<v8::Value> ThresholdImage(const v8::Arguments& args);
    static v8::Handle<v8::Value> FindRegions(const v8::Arguments& args);
    static v8::Handle<v8::Value> FindParagraphs(const v8::Arguments &args);
    static v8::Handle<v8::Value> FindTextLines(const v8::Arguments& args);
    static v8::Handle<v8::Value> FindWords(const v8::Arguments& args);
    static v8::Handle<v8::Value> FindSymbols(const v8::Arguments& args);
    static v8::Handle<v8::Value> FindText(const v8::Arguments& args);

    Tesseract(const char *datapath, const char *language);
    ~Tesseract();

    v8::Handle<v8::Value> TransformResult(tesseract::PageIteratorLevel level, const v8::Arguments &args);

    tesseract::TessBaseAPI api_;
    v8::Persistent<v8::Object> image_;
    v8::Persistent<v8::Object> rectangle_;
};

}

#endif
