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
#ifndef ZXING_H
#define ZXING_H

#include <v8.h>
#include <node.h>
#include <zxing/MultiFormatReader.h>
#include <zxing/multi/MultipleBarcodeReader.h>

class ZXing : public node::ObjectWrap
{
public:
    static void Init(v8::Handle<v8::Object> target);

private:
    static v8::Handle<v8::Value> New(const v8::Arguments& args);

    // Accessors.
    static v8::Handle<v8::Value> GetImage(v8::Local<v8::String> prop, const v8::AccessorInfo &info);
    static void SetImage(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::AccessorInfo &info);

    // Methods.
    static v8::Handle<v8::Value> FindCode(const v8::Arguments& args);
    static v8::Handle<v8::Value> FindCodes(const v8::Arguments& args);

    ZXing();
    ~ZXing();

    zxing::Ref<zxing::MultiFormatReader> reader_;
    zxing::Ref<zxing::multi::MultipleBarcodeReader> multiReader_;
    v8::Persistent<v8::Object> image_;
};

#endif
