#include <node.h>
#include "image.h"
#include "tesseract.h"
#include "zxing.h"

using namespace v8;

extern "C" void init(Handle<Object> target) 
{
    Image::Init(target);
    Tesseract::Init(target);
    ZXing::Init(target);
}

NODE_MODULE(dvBinding, init)
