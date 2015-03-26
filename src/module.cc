/*
 * node-dv - Document Vision for node.js
 *
 * Copyright (c) 2012 Christoph Schulz
 * Copyright (c) 2013-2015 creatale GmbH, contributors listed under AUTHORS
 * 
 * MIT License <https://github.com/creatale/node-dv/blob/master/LICENSE>
 */
#include <node.h>
#include "image.h"
#include "tesseract.h"
#include "zxing.h"

using namespace v8;

extern "C" void init(Handle<Object> target) 
{
    binding::Image::Init(target);
    binding::Tesseract::Init(target);
    binding::ZXing::Init(target);
}

NODE_MODULE(dvBinding, init)
