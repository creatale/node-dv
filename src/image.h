/*
 * node-dv - Document Vision for node.js
 *
 * Copyright (c) 2012 Christoph Schulz
 * Copyright (c) 2013-2015 creatale GmbH, contributors listed under AUTHORS
 * 
 * MIT License <https://github.com/creatale/node-dv/blob/master/LICENSE>
 */
#ifndef IMAGE_H
#define IMAGE_H

//#include <v8.h>
//#include <node.h>
#include <nan.h>
#include <allheaders.h>

namespace binding {

class Image : public Nan::ObjectWrap
{
public:
    static Nan::Persistent<v8::FunctionTemplate> constructor_template;

    static bool HasInstance(v8::Handle<v8::Value> val);
    static Pix *Pixels(v8::Handle<v8::Object> obj);

	static NAN_MODULE_INIT(Init);
    //static void Init(v8::Handle<v8::Object> target);

    //static v8::Handle<v8::Value> New(Pix *pix);
    static v8::Local<v8::Object> New(Pix *pix);

private:
    static NAN_METHOD(New);

    // Accessors.
    static NAN_GETTER(GetWidth);
    static NAN_GETTER(GetHeight);
    static NAN_GETTER(GetDepth);

    // Methods.
    static NAN_METHOD(Invert);
    static NAN_METHOD(Or);
    static NAN_METHOD(And);
    static NAN_METHOD(Xor);
    static NAN_METHOD(Add);
    static NAN_METHOD(Subtract);
    static NAN_METHOD(Convolve);
    static NAN_METHOD(Unsharp);
    static NAN_METHOD(Rotate);
    static NAN_METHOD(Scale);
    static NAN_METHOD(Crop);
    static NAN_METHOD(InRange);
    static NAN_METHOD(Histogram);
    static NAN_METHOD(Projection);
    static NAN_METHOD(SetMasked);
    static NAN_METHOD(ApplyCurve);
    static NAN_METHOD(RankFilter);
    static NAN_METHOD(OctreeColorQuant);
    static NAN_METHOD(MedianCutQuant);
    static NAN_METHOD(Threshold);
    static NAN_METHOD(ToGray);
    static NAN_METHOD(ToColor);
    static NAN_METHOD(ToHSV);
    static NAN_METHOD(ToRGB);
    /*static NAN_METHOD(Erode);
    static NAN_METHOD(Dilate);
    static NAN_METHOD(Open);
    static NAN_METHOD(Close);
    static NAN_METHOD(Thin);
    static NAN_METHOD(MaxDynamicRange);
    static NAN_METHOD(OtsuAdaptiveThreshold);
    static NAN_METHOD(LineSegments);
    static NAN_METHOD(FindSkew);
    static NAN_METHOD(ConnectedComponents);
    static NAN_METHOD(DistanceFunction);
    static NAN_METHOD(ClearBox);
    static NAN_METHOD(FillBox);
    static NAN_METHOD(DrawBox);
    static NAN_METHOD(DrawLine);
    static NAN_METHOD(DrawImage);*/
    static NAN_METHOD(ToBuffer);

    Image(Pix *pix);
    ~Image();

    int size() const;

    Pix *pix_;
};

}

#endif
