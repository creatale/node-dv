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

#include <v8.h>
#include <node.h>
#include <allheaders.h>

namespace binding {

class Image : public node::ObjectWrap
{
public:
    static v8::Persistent<v8::FunctionTemplate> constructor_template;

    static bool HasInstance(v8::Handle<v8::Value> val);
    static Pix *Pixels(v8::Handle<v8::Object> obj);

    static void Init(v8::Handle<v8::Object> target);

    static v8::Handle<v8::Value> New(Pix *pix);

private:
    static v8::Handle<v8::Value> New(const v8::Arguments& args);

    // Accessors.
    static v8::Handle<v8::Value> GetWidth(v8::Local<v8::String> prop, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> GetHeight(v8::Local<v8::String> prop, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> GetDepth(v8::Local<v8::String> prop, const v8::AccessorInfo &info);

    // Methods.
    static v8::Handle<v8::Value> Invert(const v8::Arguments& args);
    static v8::Handle<v8::Value> Or(const v8::Arguments& args);
    static v8::Handle<v8::Value> And(const v8::Arguments& args);
    static v8::Handle<v8::Value> Xor(const v8::Arguments& args);
    static v8::Handle<v8::Value> Add(const v8::Arguments& args);
    static v8::Handle<v8::Value> Subtract(const v8::Arguments& args);
    static v8::Handle<v8::Value> Convolve(const v8::Arguments& args);
    static v8::Handle<v8::Value> Unsharp(const v8::Arguments& args);
    static v8::Handle<v8::Value> Rotate(const v8::Arguments& args);
    static v8::Handle<v8::Value> Scale(const v8::Arguments& args);
    static v8::Handle<v8::Value> Crop(const v8::Arguments& args);
    static v8::Handle<v8::Value> InRange(const v8::Arguments& args);
    static v8::Handle<v8::Value> Histogram(const v8::Arguments &args);
    static v8::Handle<v8::Value> Projection(const v8::Arguments &args);
    static v8::Handle<v8::Value> SetMasked(const v8::Arguments &args);
    static v8::Handle<v8::Value> ApplyCurve(const v8::Arguments &args);
    static v8::Handle<v8::Value> RankFilter(const v8::Arguments& args);
    static v8::Handle<v8::Value> OctreeColorQuant(const v8::Arguments& args);
    static v8::Handle<v8::Value> MedianCutQuant(const v8::Arguments& args);
    static v8::Handle<v8::Value> Threshold(const v8::Arguments& args);
    static v8::Handle<v8::Value> ToGray(const v8::Arguments& args);
    static v8::Handle<v8::Value> ToColor(const v8::Arguments& args);
    static v8::Handle<v8::Value> ToHSV(const v8::Arguments& args);
    static v8::Handle<v8::Value> ToRGB(const v8::Arguments& args);
    static v8::Handle<v8::Value> Erode(const v8::Arguments& args);
    static v8::Handle<v8::Value> Dilate(const v8::Arguments& args);
    static v8::Handle<v8::Value> Open(const v8::Arguments& args);
    static v8::Handle<v8::Value> Close(const v8::Arguments& args);
    static v8::Handle<v8::Value> Thin(const v8::Arguments& args);
    static v8::Handle<v8::Value> MaxDynamicRange(const v8::Arguments &args);
    static v8::Handle<v8::Value> OtsuAdaptiveThreshold(const v8::Arguments& args);
    static v8::Handle<v8::Value> LineSegments(const v8::Arguments& args);
    static v8::Handle<v8::Value> FindSkew(const v8::Arguments& args);
    static v8::Handle<v8::Value> ConnectedComponents(const v8::Arguments& args);
    static v8::Handle<v8::Value> DistanceFunction(const v8::Arguments& args);
    static v8::Handle<v8::Value> ClearBox(const v8::Arguments& args);
    static v8::Handle<v8::Value> FillBox(const v8::Arguments &args);
    static v8::Handle<v8::Value> DrawBox(const v8::Arguments& args);
    static v8::Handle<v8::Value> DrawLine(const v8::Arguments& args);
    static v8::Handle<v8::Value> DrawImage(const v8::Arguments& args);
    static v8::Handle<v8::Value> ToBuffer(const v8::Arguments& args);

    Image(Pix *pix);
    ~Image();

    int size() const;

    Pix *pix_;
};

}

#endif
