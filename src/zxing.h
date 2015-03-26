/*
 * node-dv - Document Vision for node.js
 *
 * Copyright (c) 2012 Christoph Schulz
 * Copyright (c) 2013-2015 creatale GmbH, contributors listed under AUTHORS
 * 
 * MIT License <https://github.com/creatale/node-dv/blob/master/LICENSE>
 */
#ifndef ZXING_H
#define ZXING_H

#include <v8.h>
#include <node.h>
#include <zxing/DecodeHints.h>
#include <zxing/MultiFormatReader.h>

namespace binding {

class ZXing : public node::ObjectWrap
{
public:
    static void Init(v8::Handle<v8::Object> target);

private:
    static v8::Handle<v8::Value> New(const v8::Arguments& args);

    // Accessors.
    static v8::Handle<v8::Value> GetImage(v8::Local<v8::String> prop, const v8::AccessorInfo &info);
    static void SetImage(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> GetFormats(v8::Local<v8::String> prop, const v8::AccessorInfo &info);
    static void SetFormats(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> GetTryHarder(v8::Local<v8::String> prop, const v8::AccessorInfo &info);
    static void SetTryHarder(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::AccessorInfo &info);

    // Methods.
    static v8::Handle<v8::Value> FindCode(const v8::Arguments& args);

    ZXing();
    ~ZXing();

    static const zxing::BarcodeFormat::Value BARCODEFORMATS[];
    static const size_t BARCODEFORMATS_LENGTH;

    v8::Persistent<v8::Object> image_;
    zxing::DecodeHints hints_;
    zxing::Ref<zxing::MultiFormatReader> reader_;
};

}

#endif
