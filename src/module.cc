#include <node.h>
#include "image.h"
#include "tesseract.h"
#include "zbar.h"

using namespace v8;

extern "C" void init(Handle<Object> target) 
{
    Image::Init(target);
    Tesseract::Init(target);
    ZBar::Init(target);
}

NODE_MODULE(dvBinding, init)
