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
#ifndef TESSERACT_H
#define TESSERACT_H

#include <v8.h>
#include <node.h>
#include <baseapi.h>

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

#endif
