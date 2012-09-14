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
    static v8::Handle<v8::Value> Write(const v8::Arguments& args);

    Image(Pix *pix);
    ~Image();

    int size() const;

    Pix *pix_;
};

#endif
