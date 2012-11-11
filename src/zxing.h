#ifndef ZXING_H
#define ZXING_H

#include <v8.h>
#include <node.h>
#include <zxing/MultiFormatReader.h>

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

    ZXing();
    ~ZXing();

    zxing::Ref<zxing::MultiFormatReader> reader_;
    v8::Persistent<v8::Object> image_;
};

#endif
