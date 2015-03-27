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
#include <nan.h>
#include <zxing/DecodeHints.h>
#include <zxing/MultiFormatReader.h>

namespace binding {

class ZXing : public node::ObjectWrap
{
public:
    static void Init(v8::Handle<v8::Object> target);

private:
    static NAN_METHOD(New);

    // Accessors.
    static NAN_GETTER(GetImage);
    static NAN_SETTER(SetImage);
    static NAN_GETTER(GetFormats);
    static NAN_SETTER(SetFormats);
    static NAN_GETTER(GetTryHarder);
    static NAN_SETTER(SetTryHarder);

    // Methods.
    static NAN_METHOD(FindCode);

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
