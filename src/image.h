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
    static v8::Handle<v8::Value> DrawBox(const v8::Arguments& args);
    static v8::Handle<v8::Value> ToBuffer(const v8::Arguments& args);

    Image(Pix *pix);
    ~Image();

    int size() const;

    Pix *pix_;
};

#endif
