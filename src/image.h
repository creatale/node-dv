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
#ifndef IMAGE_H
#define IMAGE_H

#include <v8.h>
#include <node.h>
#include <allheaders.h>

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
    static v8::Handle<v8::Value> Subtract(const v8::Arguments& args);
    static v8::Handle<v8::Value> Rotate(const v8::Arguments& args);
    static v8::Handle<v8::Value> Scale(const v8::Arguments& args);
    static v8::Handle<v8::Value> Crop(const v8::Arguments& args);
    static v8::Handle<v8::Value> RankFilter(const v8::Arguments& args);
    static v8::Handle<v8::Value> ToGray(const v8::Arguments& args);
    static v8::Handle<v8::Value> Erode(const v8::Arguments& args);
    static v8::Handle<v8::Value> Dilate(const v8::Arguments& args);
    static v8::Handle<v8::Value> OtsuAdaptiveThreshold(const v8::Arguments& args);
    static v8::Handle<v8::Value> FindSkew(const v8::Arguments& args);
    static v8::Handle<v8::Value> ConnectedComponents(const v8::Arguments& args);
    static v8::Handle<v8::Value> ClearBox(const v8::Arguments& args);
    static v8::Handle<v8::Value> DrawBox(const v8::Arguments& args);
    static v8::Handle<v8::Value> ToBuffer(const v8::Arguments& args);

    Image(Pix *pix);
    ~Image();

    int size() const;

    Pix *pix_;
};

#endif
