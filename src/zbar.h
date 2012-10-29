#ifndef ZBAR_H
#define ZBAR_H

#include <v8.h>
#include <node.h>
extern "C" {
#undef __cplusplus
#include <include/zbar.h>
#define __cplusplus
}

class ZBar : public node::ObjectWrap
{
public:
    static void Init(v8::Handle<v8::Object> target);

private:
    static v8::Handle<v8::Value> New(const v8::Arguments& args);

    // Accessors.
    static v8::Handle<v8::Value> GetImage(v8::Local<v8::String> prop, const v8::AccessorInfo &info);
    static void SetImage(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::AccessorInfo &info);

    // Methods.
    static v8::Handle<v8::Value> FindSymbols(const v8::Arguments& args);

    ZBar();
    ~ZBar();
    void SetZImage(v8::Handle<v8::Object> image);

    zbar_image_scanner_t *zScanner_;
    zbar_image_t *zImage_;
    v8::Persistent<v8::Object> image_;
};

#endif
