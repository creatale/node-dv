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
#include <nan.h>
#include <baseapi.h>

namespace binding {

class Tesseract : public node::ObjectWrap
{
public:
    static void Init(v8::Handle<v8::Object> target);

private:
    static NAN_METHOD(New);

    // Accessors.
    static NAN_GETTER(GetImage);
    static NAN_SETTER(SetImage);
    static NAN_GETTER(GetRectangle);
    static NAN_SETTER(SetRectangle);
    static NAN_GETTER(GetPageSegMode);
    static NAN_SETTER(SetPageSegMode);
    static NAN_GETTER(GetSymbolWhitelist);
    static NAN_SETTER(SetSymbolWhitelist);
    static NAN_SETTER(SetVariable);
    static NAN_GETTER(GetIntVariable);
    static NAN_GETTER(GetBoolVariable);
    static NAN_GETTER(GetDoubleVariable);
    static NAN_GETTER(GetStringVariable);

    // Methods.
    static NAN_METHOD(Clear);
    static NAN_METHOD(ClearAdaptiveClassifier);
    static NAN_METHOD(ThresholdImage);
    static NAN_METHOD(FindRegions);
    static NAN_METHOD(FindParagraphs);
    static NAN_METHOD(FindTextLines);
    static NAN_METHOD(FindWords);
    static NAN_METHOD(FindSymbols);
    static NAN_METHOD(FindText);

    Tesseract(const char *datapath, const char *language);
    ~Tesseract();

    _NAN_METHOD_RETURN_TYPE TransformResult(tesseract::PageIteratorLevel level, _NAN_METHOD_ARGS);

    tesseract::TessBaseAPI api_;
    v8::Persistent<v8::Object> image_;
    v8::Persistent<v8::Object> rectangle_;
};

}

#endif
