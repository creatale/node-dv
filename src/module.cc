#include <node.h>
#include "image.h"
#include "tesseract.h"
#include "zxing.h"
#include "omr.h"

using namespace v8;

extern "C" void init(Handle<Object> target) 
{
    Image::Init(target);
    Tesseract::Init(target);
    ZXing::Init(target);
    OMR::Init(target);
}

NODE_MODULE(dvBinding, init)
