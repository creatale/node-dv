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
#include "tickreader.h"
#include "image.h"
#include <vector>
#include <algorithm>
#include <iostream>

using namespace v8;
using namespace node;

#define THROW(type, msg) \
    ThrowException(Exception::type(String::New(msg)))

#define TICKREADER_NOTHRESHOLD -1
#define NUM_INTENSITY_LEVELS 256
#define MAX_INTENSITY_LEVEL NUM_INTENSITY_LEVELS - 1

void getHistogram(PIX *pix8, std::vector<unsigned int> &histogram) {
    uint32_t *line;
    line = pix8->data;
    histogram.assign(NUM_INTENSITY_LEVELS, 0);
    for (uint32_t y = 0; y < pix8->h; ++y) {
        for (uint32_t x = 0; x < pix8->w; ++x) {
            histogram[GET_DATA_BYTE(line, x)]++;
        }
        line += pix8->wpl;
    }
}

int findOtsuThreshold(std::vector<unsigned int> &histogram) {
    int result = 0;
    double maximumVariance = 0.0f;
    unsigned int totalNumberOfPixels = 0;
    unsigned int totalIntensity = 0;
    unsigned int backgroundIntensity = 0;

    for (size_t i = 0; i < histogram.size(); i++) {
        totalNumberOfPixels += histogram[i];
        totalIntensity += i * histogram[i];
    }

    unsigned int weightBackground = 0;
    unsigned int weightForeground = totalNumberOfPixels;

    for (unsigned int threshold = 0; threshold < histogram.size() && weightForeground > 0; threshold++) {
        weightBackground += histogram[threshold];
        if (weightBackground > 0) {
            weightForeground = totalNumberOfPixels - weightBackground;
            if (weightForeground > 0) {
                backgroundIntensity += threshold * histogram[threshold];

                double meanBackground = static_cast<double>(backgroundIntensity) / static_cast<double>(weightBackground);
                double meanForeground = static_cast<double>(totalIntensity - backgroundIntensity) / static_cast<double>(weightForeground);

                double varianceBetweenClasses = static_cast<double>(weightBackground) * static_cast<double>(weightForeground) * (meanBackground - meanForeground) * (meanBackground - meanForeground);

                if (varianceBetweenClasses > maximumVariance) {
                    maximumVariance = varianceBetweenClasses;
                    result = static_cast<int>(threshold);
                }
            }
        }
    }

    return result;
}

void getHorizontalProjection(PIX *pix8, int threshold, std::vector<int> &values, std::vector<int> &slopes) {
    uint32_t *line;
    line = pix8->data;
    values.assign(pix8->h, 0);
    for (uint32_t y = 0; y < pix8->h; ++y) {
        for (uint32_t x = 0; x < pix8->w; ++x) {
            values[y] += (GET_DATA_BYTE(line, x) <= threshold ? 1 : 0);
        }
        line += pix8->wpl;
    }

    if (values.size() > 0) {
        slopes.assign(values.size() - 1, 0);

        for (size_t i = 0; i < slopes.size(); i++) {
            slopes[i] = values[i + 1] - values[i];
        }
    } else {
        slopes.resize(0);
    }
}

void getVerticalProjection(PIX *pix8, int threshold, std::vector<int> &values, std::vector<int> &slopes) {
    uint32_t *line;
    line = pix8->data;
    values.assign(pix8->w, 0);
    for (uint32_t y = 0; y < pix8->h; ++y) {
        for (uint32_t x = 0; x < pix8->w; ++x) {
            values[x] += (GET_DATA_BYTE(line, x) <= threshold ? 1 : 0);
        }
        line += pix8->wpl;
    }

    if (values.size() > 0) {
        slopes.assign(values.size() - 1, 0);

        for (size_t i = 0; i < slopes.size(); i++) {
            slopes[i] = values[i + 1] - values[i];
        }
    } else {
        slopes.resize(0);
    }
}

void locatePeaks(std::vector<int> &projection, std::vector<int> &slopes, unsigned int minPeakDistance, unsigned int minPeakHeight, std::vector<int> &upPeaks, std::vector<int> &downPeaks) {
    upPeaks.resize(0);
    downPeaks.resize(0);
    for (size_t i = 0; i < slopes.size(); i++) {
        if (slopes[i] > 0) {
            if (upPeaks.size() > 0) {
                if (upPeaks.back() + minPeakDistance >= i) {
                    if (slopes[upPeaks.back()] <= slopes[i]
                            /*&& abs(slopes[i]) >= static_cast<int>(minPeakHeight)*/) {
                        upPeaks.back() = i;
                    }
                } else {
                    upPeaks.push_back(i);
                }
            } else {
                upPeaks.push_back(i);
            }
        } else if (slopes[i] < 0) {
            if (downPeaks.size() > 0) {
                if (downPeaks.back() + minPeakDistance >= i) {
                    if (slopes[downPeaks.back()] >= slopes[i]
                            /*&& abs(slopes[i]) >= static_cast<int>(minPeakHeight)*/) {
                        downPeaks.back() = i;
                    }
                } else {
                    downPeaks.push_back(i);
                }
            } else {
                downPeaks.push_back(i);
            }
        } // else slopes[i] is 0, nothing to be done yet
    }
}

bool isInImageBounds(PIX *pix8, int left, int right, int top, int bottom) {
    return left >= 0 && right < static_cast<int>(pix8->w) && top >= 0 && bottom < static_cast<int>(pix8->h);
}

uint32_t getFill(PIX *pix8, int left, int right, int top, int bottom, int threshold) {
    uint32_t result = 0;
    if (isInImageBounds(pix8, left, right, top, bottom)) {
        uint32_t *line;
        line = pix8->data + top * pix8->wpl;
        for (int y = top; y <= bottom; ++y) {
            for (int x = left; x <= right; ++x) {
                result += (GET_DATA_BYTE(line, x) <= threshold ? 1 : 0);
            }
            line += pix8->wpl;
        }
    }
    return result;
}

double getFillRatio(PIX *pix8, int left, int right, int top, int bottom, int threshold) {
    uint32_t fill = getFill(pix8, left, right, top, bottom, threshold);

    int numPixels = (right - left + 1) * (bottom - top + 1);

    if (numPixels > 0) {
        return static_cast<double>(fill) / static_cast<double>(numPixels);
    } else {
        return 0.0f;
    }
}

double calculateCheckedConfidence(double fillRatio, double checkedThreshold, double checkedTrueMargin, double checkedFalseMargin) {
    if (fillRatio < checkedThreshold - checkedFalseMargin) {
        return 0.0f;
    } else if(fillRatio < checkedThreshold) {
        return (fillRatio - checkedThreshold + checkedFalseMargin) * 0.5f / checkedFalseMargin;
    } else if(fillRatio < checkedThreshold + checkedTrueMargin) {
        return 1.0f + (fillRatio - checkedThreshold - checkedTrueMargin) * 0.5f / checkedTrueMargin;
    } else {
        return 1.0f;
    }
}

void checkboxIsChecked(
        PIX *pix8,
        std::vector<int> &horizontalUpPeaks,
        std::vector<int> &horizontalDownPeaks,
        std::vector<int> &verticalUpPeaks,
        std::vector<int> &verticalDownPeaks,
        unsigned int minPeakDistance,
        int threshold,
        double outerCheckedThreshold,
        double outerCheckedTrueMargin,
        double outerCheckedFalseMargin,
        double innerCheckedThreshold,
        double innerCheckedTrueMargin,
        double innerCheckedFalseMargin,
        bool &checked,
        double &confidence,
        std::vector<Local<Object> > &debugs) {
    // for now assume, that there is only one checkbox in the image
    Local<Object> debug = Object::New();
    debugs.push_back(debug);
    Local<Array> horizontalUpPeakArray = Array::New();
    for (size_t i = 0; i < horizontalUpPeaks.size(); i++) {
        horizontalUpPeakArray->Set(i, Number::New(horizontalUpPeaks[i]));
    }
    Local<Array> horizontalDownPeakArray = Array::New();
    for (size_t i = 0; i < horizontalDownPeaks.size(); i++) {
        horizontalDownPeakArray->Set(i, Number::New(horizontalDownPeaks[i]));
    }
    Local<Array> verticalUpPeakArray = Array::New();
    for (size_t i = 0; i < verticalUpPeaks.size(); i++) {
        verticalUpPeakArray->Set(i, Number::New(verticalUpPeaks[i]));
    }
    Local<Array> verticalDownPeakArray = Array::New();
    for (size_t i = 0; i < verticalDownPeaks.size(); i++) {
        verticalDownPeakArray->Set(i, Number::New(verticalDownPeaks[i]));
    }
    debug->Set(String::NewSymbol("horizontalUpPeaks"), horizontalUpPeakArray);
    debug->Set(String::NewSymbol("horizontalDownPeaks"), horizontalDownPeakArray);
    debug->Set(String::NewSymbol("verticalUpPeaks"), verticalUpPeakArray);
    debug->Set(String::NewSymbol("verticalDownPeaks"), verticalDownPeakArray);
    debug->Set(String::NewSymbol("minPeakDistance"), Number::New(minPeakDistance));
    debug->Set(String::NewSymbol("threshold"), Number::New(threshold));
    debug->Set(String::NewSymbol("outerCheckedThreshold"), Number::New(outerCheckedThreshold));
    debug->Set(String::NewSymbol("outerCheckedTrueMargin"), Number::New(outerCheckedTrueMargin));
    debug->Set(String::NewSymbol("outerCheckedFalseMargin"), Number::New(outerCheckedFalseMargin));
    debug->Set(String::NewSymbol("innerCheckedThreshold"), Number::New(innerCheckedThreshold));
    debug->Set(String::NewSymbol("innerCheckedTrueMargin"), Number::New(innerCheckedTrueMargin));
    debug->Set(String::NewSymbol("innerCheckedFalseMargin"), Number::New(innerCheckedFalseMargin));
    if (horizontalUpPeaks.size() > 0 && horizontalDownPeaks.size() > 0 && verticalUpPeaks.size() > 0 && verticalDownPeaks.size() > 0) {
        if (horizontalUpPeaks[0] + static_cast<int>(minPeakDistance) > horizontalDownPeaks[0] && verticalUpPeaks[0] + static_cast<int>(minPeakDistance) > verticalDownPeaks[0]
                && horizontalUpPeaks.size() > 1 && horizontalDownPeaks.size() > 1 && verticalUpPeaks.size() > 1 && verticalDownPeaks.size() > 1) {
            double outerFillRatio = getFillRatio(pix8, verticalUpPeaks[0] + 1, verticalDownPeaks[1] - 1, horizontalUpPeaks[0] + 1, horizontalDownPeaks[1] - 1, threshold);
            double innerFillRatio = getFillRatio(pix8, verticalDownPeaks[0] + 1, verticalUpPeaks[1] - 1, horizontalDownPeaks[0] + 1, horizontalUpPeaks[1] - 1, threshold);
            debug->Set(String::NewSymbol("outerFillRatio"), Number::New(outerFillRatio));
            debug->Set(String::NewSymbol("innerFillRatio"), Number::New(innerFillRatio));
            if (outerFillRatio > outerCheckedThreshold) {
                checked = true;
                confidence = calculateCheckedConfidence(outerFillRatio, outerCheckedThreshold, outerCheckedTrueMargin, outerCheckedFalseMargin);
            } else {
                checked = innerFillRatio > innerCheckedThreshold;
                confidence = checked ?
                            calculateCheckedConfidence(innerFillRatio, innerCheckedThreshold, innerCheckedTrueMargin, innerCheckedFalseMargin) :
                            1.0 - calculateCheckedConfidence(innerFillRatio, innerCheckedThreshold, innerCheckedTrueMargin, innerCheckedFalseMargin);
            }
        } else {
            if (horizontalUpPeaks[0] < horizontalDownPeaks[0] && verticalUpPeaks[0] < verticalDownPeaks[0]) {
                double fillRatio = getFillRatio(pix8, horizontalUpPeaks[0] + 1, horizontalDownPeaks[0] - 1, verticalUpPeaks[0] + 1, verticalDownPeaks[0] - 1, threshold);
                debug->Set(String::NewSymbol("fillRatio"), Number::New(fillRatio));
                checked = fillRatio > outerCheckedThreshold;
                confidence = checked ?
                            calculateCheckedConfidence(fillRatio, outerCheckedThreshold, outerCheckedTrueMargin, outerCheckedFalseMargin) :
                            1.0 - calculateCheckedConfidence(fillRatio, outerCheckedThreshold, outerCheckedTrueMargin, outerCheckedFalseMargin);
            } else {
                // something went wrong
                checked = false;
                confidence = 0.0;
            }
        }
    } else {
        double fillRatio = getFillRatio(pix8, 0, pix8->w - 1, 0, pix8->h - 1, threshold);
        debug->Set(String::NewSymbol("fillRatio"), Number::New(fillRatio));
        checked = fillRatio > outerCheckedThreshold;
        confidence = checked ?
                    calculateCheckedConfidence(fillRatio, outerCheckedThreshold, outerCheckedTrueMargin, outerCheckedFalseMargin) :
                    1.0 - calculateCheckedConfidence(fillRatio, outerCheckedThreshold, outerCheckedTrueMargin, outerCheckedFalseMargin);
    }
}

// use div(grad(img)) for detection?

void TickReader::Init(Handle<Object> target)
{
    Local<FunctionTemplate> constructor_template = FunctionTemplate::New(New);
    constructor_template->SetClassName(String::NewSymbol("TickReader"));
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    Local<ObjectTemplate> proto = constructor_template->PrototypeTemplate();
    proto->Set(String::NewSymbol("getHorizontalProjection"),
               FunctionTemplate::New(GetHorizontalProjection)->GetFunction());
    proto->Set(String::NewSymbol("getVerticalProjection"),
               FunctionTemplate::New(GetVerticalProjection)->GetFunction());
    proto->Set(String::NewSymbol("locatePeaks"),
               FunctionTemplate::New(LocatePeaks)->GetFunction());
    proto->Set(String::NewSymbol("getFill"),
               FunctionTemplate::New(GetFill)->GetFunction());
    proto->Set(String::NewSymbol("getFillRatio"),
               FunctionTemplate::New(GetFillRatio)->GetFunction());
    proto->Set(String::NewSymbol("checkboxIsChecked"),
               FunctionTemplate::New(CheckboxIsChecked)->GetFunction());
    target->Set(String::NewSymbol("TickReader"),
                Persistent<Function>::New(constructor_template->GetFunction()));
}

Handle<Value> TickReader::New(const Arguments &args)
{
    HandleScope scope;
    double outerCheckedThreshold = 0.5f;
    double outerCheckedTrueMargin = 0.1f;
    double outerCheckedFalseMargin = 0.1f;
    double innerCheckedThreshold = 0.04f;
    double innerCheckedTrueMargin = 0.03f;
    double innerCheckedFalseMargin = 0.03f;
    if (args.Length() == 1 && args[0]->IsObject()) {
        Local<Object> params = args[0]->ToObject();
        Local<Value> outerCheckedThresholdValue = params->Get(String::NewSymbol("outerCheckedThreshold"));
        if (outerCheckedThresholdValue->IsNumber()) {
            outerCheckedThreshold = outerCheckedThresholdValue->NumberValue();
        }
        Local<Value> outerCheckedTrueMarginValue = params->Get(String::NewSymbol("outerCheckedTrueMargin"));
        if (outerCheckedTrueMarginValue->IsNumber()) {
            outerCheckedTrueMargin = outerCheckedTrueMarginValue->NumberValue();
        }
        Local<Value> outerCheckedFalseMarginValue = params->Get(String::NewSymbol("outerCheckedFalseMargin"));
        if (outerCheckedFalseMarginValue->IsNumber()) {
            outerCheckedFalseMargin = outerCheckedFalseMarginValue->NumberValue();
        }
        Local<Value> innerCheckedThresholdValue = params->Get(String::NewSymbol("innerCheckedThreshold"));
        if (innerCheckedThresholdValue->IsNumber()) {
            innerCheckedThreshold = innerCheckedThresholdValue->NumberValue();
        }
        Local<Value> innerCheckedTrueMarginValue = params->Get(String::NewSymbol("innerCheckedTrueMargin"));
        if (innerCheckedTrueMarginValue->IsNumber()) {
            innerCheckedTrueMargin = innerCheckedTrueMarginValue->NumberValue();
        }
        Local<Value> innerCheckedFalseMarginValue = params->Get(String::NewSymbol("innerCheckedFalseMargin"));
        if (innerCheckedFalseMarginValue->IsNumber()) {
            innerCheckedFalseMargin = innerCheckedFalseMarginValue->NumberValue();
        }
    }
    TickReader *obj = new TickReader(outerCheckedThreshold, outerCheckedTrueMargin, outerCheckedFalseMargin, innerCheckedThreshold, innerCheckedTrueMargin, innerCheckedFalseMargin);
    obj->Wrap(args.This());
    return args.This();
}

Handle<Value> TickReader::GetHorizontalProjection(const Arguments &args) {
    HandleScope scope;
    if (args.Length() == 1 && Image::HasInstance(args[0])) {
        Pix *pix = Image::Pixels(args[0]->ToObject());
        if (pix->d <= 8) {
            // work on grayscale images
            PIX *pix8 = pixConvertTo8(pix, 0);
            std::vector<unsigned int> histogram;
            std::vector<int> values;
            std::vector<int> slopes;
            getHistogram(pix8, histogram);
            int threshold = findOtsuThreshold(histogram);
            getHorizontalProjection(pix8, threshold, values, slopes);
            pixDestroy(&pix8);
            Local<Object> result = Object::New();
            Local<Array> valuesArray = Array::New();
            for (size_t i = 0; i < values.size(); i++) {
                valuesArray->Set(i, Number::New(values[i]));
            }
            Local<Array> slopesArray = Array::New();
            for (size_t i = 0; i < slopes.size(); i++) {
                slopesArray->Set(i, Number::New(slopes[i]));
            }
            result->Set(String::NewSymbol("values"), valuesArray);
            result->Set(String::NewSymbol("slopes"), slopesArray);
            return scope.Close(result);
        } else {
            return THROW(Error, "color depth of 8 bits or less expected");
        }
    } else {
        return THROW(TypeError, "expected (Image) signature");
    }
}

Handle<Value> TickReader::GetVerticalProjection(const Arguments &args) {
    HandleScope scope;
    if (args.Length() == 1 && Image::HasInstance(args[0])) {
        Pix *pix = Image::Pixels(args[0]->ToObject());
        if (pix->d <= 8) {
            // work on grayscale images
            PIX *pix8 = pixConvertTo8(pix, 0);
            std::vector<unsigned int> histogram;
            std::vector<int> values;
            std::vector<int> slopes;
            getHistogram(pix8, histogram);
            int threshold = findOtsuThreshold(histogram);
            getVerticalProjection(pix8, threshold, values, slopes);
            pixDestroy(&pix8);
            Local<Object> result = Object::New();
            Local<Array> valuesArray = Array::New();
            for (size_t i = 0; i < values.size(); i++) {
                valuesArray->Set(i, Number::New(values[i]));
            }
            Local<Array> slopesArray = Array::New();
            for (size_t i = 0; i < slopes.size(); i++) {
                slopesArray->Set(i, Number::New(slopes[i]));
            }
            result->Set(String::NewSymbol("values"), valuesArray);
            result->Set(String::NewSymbol("slopes"), slopesArray);
            return scope.Close(result);
        } else {
            return THROW(Error, "color depth of 8 bits or less expected");
        }
    } else {
        return THROW(TypeError, "expected (Image) signature");
    }
}

Handle<Value> TickReader::LocatePeaks(const Arguments &args) {
    HandleScope scope;
    if (args.Length() == 4 && args[0]->IsArray() && args[1]->IsArray() && args[2]->IsNumber() && args[3]->IsNumber()) {
        Local<Object> projection = args[0]->ToObject();
        Local<Object> slopes = args[1]->ToObject();
        unsigned int minPeakDistance = args[2]->Uint32Value();
        unsigned int minPeakHeight = args[3]->Uint32Value();

        std::vector<int> projectionVector(projection->Get(String::NewSymbol("length"))->Uint32Value());
        std::vector<int> slopesVector(slopes->Get(String::NewSymbol("length"))->Uint32Value());

        for (size_t i = 0; i < projectionVector.size(); i++) {
            projectionVector[i] = projection->Get(i)->Int32Value();
        }
        for (size_t i = 0; i < slopesVector.size(); i++) {
            slopesVector[i] = slopes->Get(i)->Int32Value();
        }

        std::vector<int> upPeaks;
        std::vector<int> downPeaks;
        locatePeaks(projectionVector, slopesVector, minPeakDistance, minPeakHeight, upPeaks, downPeaks);

        Local<Object> result = Object::New();
        Local<Array> upPeaksArray = Array::New();
        Local<Array> downPeaksArray = Array::New();
        for (size_t i = 0; i < upPeaks.size(); i++) {
            upPeaksArray->Set(i, Number::New(upPeaks[i]));
        }
        for (size_t i = 0; i < downPeaks.size(); i++) {
            downPeaksArray->Set(i, Number::New(downPeaks[i]));
        }
        result->Set(String::NewSymbol("upPeaks"), upPeaksArray);
        result->Set(String::NewSymbol("downPeaks"), downPeaksArray);
        return scope.Close(result);
    } else {
        return THROW(TypeError, "expected (Array, Array, int, int) signature");
    }
}

Handle<Value> TickReader::GetFill(const Arguments &args) {
    HandleScope scope;
    if (args.Length() == 5 && Image::HasInstance(args[0]) && args[1]->IsNumber() && args[2]->IsNumber() && args[3]->IsNumber() && args[4]->IsNumber()) {
        Pix *pix = Image::Pixels(args[0]->ToObject());
        unsigned int left = args[1]->Uint32Value();
        unsigned int right = args[2]->Uint32Value();
        unsigned int top = args[3]->Uint32Value();
        unsigned int bottom = args[4]->Uint32Value();

        if (pix->d <= 8) {
            PIX *pix8 = pixConvertTo8(pix, 0);
            std::vector<unsigned int> histogram;
            getHistogram(pix8, histogram);
            int threshold = findOtsuThreshold(histogram);
            uint32_t fill = getFill(pix8, left, right, top, bottom, threshold);
            pixDestroy(&pix8);

            Local<Number> result = Number::New(fill);
            return scope.Close(result);
        } else {
            return THROW(Error, "color depth of 8 bits or less expected");
        }
    } else {
        return THROW(TypeError, "expected (Image, int, int, int, int) signature");
    }
}

Handle<Value> TickReader::GetFillRatio(const Arguments &args) {
    HandleScope scope;
    if (args.Length() == 5 && Image::HasInstance(args[0]) && args[1]->IsNumber() && args[2]->IsNumber() && args[3]->IsNumber() && args[4]->IsNumber()) {
        Pix *pix = Image::Pixels(args[0]->ToObject());
        unsigned int left = args[1]->Uint32Value();
        unsigned int right = args[2]->Uint32Value();
        unsigned int top = args[3]->Uint32Value();
        unsigned int bottom = args[4]->Uint32Value();

        if (pix->d <= 8) {
            PIX *pix8 = pixConvertTo8(pix, 0);
            std::vector<unsigned int> histogram;
            getHistogram(pix8, histogram);
            int threshold = findOtsuThreshold(histogram);
            double fillRatio = getFillRatio(pix8, left, right, top, bottom, threshold);
            pixDestroy(&pix8);

            Local<Number> result = Number::New(fillRatio);
            return scope.Close(result);
        } else {
            return THROW(Error, "color depth of 8 bits or less expected");
        }
    } else {
        return THROW(TypeError, "expected (Image, int, int, int, int) signature");
    }
}

Handle<Value> TickReader::CheckboxIsChecked(const Arguments &args) {
    HandleScope scope;
    TickReader* thisObj = ObjectWrap::Unwrap<TickReader>(args.This());
    if (args.Length() >= 1 && Image::HasInstance(args[0])) {
        Pix *pix = Image::Pixels(args[0]->ToObject());

        if (pix->d <= 8) {
            PIX *pix8 = pixConvertTo8(pix, 0);
            unsigned int minPeakDistance = std::min<uint32_t>(pix8->h, pix8->w) / 3;
            unsigned int minPeakHeight = minPeakDistance / 3;
            std::vector<int> horizontalProjection;
            std::vector<int> horizontalProjectionSlopes;
            std::vector<int> verticalProjection;
            std::vector<int> verticalProjectionSlopes;
            std::vector<int> horizontalUpPeaks;
            std::vector<int> horizontalDownPeaks;
            std::vector<int> verticalUpPeaks;
            std::vector<int> verticalDownPeaks;
            std::vector<unsigned int> histogram;
            getHistogram(pix8, histogram);
            std::vector<Local<Object> > debugs;
            int threshold = findOtsuThreshold(histogram);
            getHorizontalProjection(pix8, threshold, horizontalProjection, horizontalProjectionSlopes);
            getVerticalProjection(pix8, threshold, verticalProjection, verticalProjectionSlopes);
            locatePeaks(horizontalProjection, horizontalProjectionSlopes, minPeakDistance, minPeakHeight, horizontalUpPeaks, horizontalDownPeaks);
            locatePeaks(verticalProjection, verticalProjectionSlopes, minPeakDistance, minPeakHeight, verticalUpPeaks, verticalDownPeaks);
            bool checked = false;
            double checkedConfidence = 1.0;
            checkboxIsChecked(
                        pix8,
                        horizontalUpPeaks,
                        horizontalDownPeaks,
                        verticalUpPeaks,
                        verticalDownPeaks,
                        minPeakDistance,
                        threshold,
                        thisObj->m_outerCheckedThreshold,
                        thisObj->m_outerCheckedTrueMargin,
                        thisObj->m_outerCheckedFalseMargin,
                        thisObj->m_innerCheckedThreshold,
                        thisObj->m_innerCheckedTrueMargin,
                        thisObj->m_innerCheckedFalseMargin,
                        checked,
                        checkedConfidence,
                        debugs);
            pixDestroy(&pix8);

            Local<Object> object = Object::New();
            object->Set(String::NewSymbol("checked"), Boolean::New(checked));
            object->Set(String::NewSymbol("confidence"), Number::New(checkedConfidence));
            object->Set(String::NewSymbol("debug"), debugs[0]);

            return scope.Close(object);
        } else {
            return THROW(Error, "color depth of 8 bits or less expected");
        }
    } else {
        return THROW(TypeError, "expected (Image) signature");
    }
}

TickReader::TickReader(double outerCheckedThreshold, double outerCheckedTrueMargin, double outerCheckedFalseMargin, double innerCheckedThreshold, double innerCheckedTrueMargin, double innerCheckedFalseMargin)
    : m_outerCheckedThreshold(outerCheckedThreshold),
      m_outerCheckedTrueMargin(outerCheckedTrueMargin),
      m_outerCheckedFalseMargin(outerCheckedFalseMargin),
      m_innerCheckedThreshold(innerCheckedThreshold),
      m_innerCheckedTrueMargin(innerCheckedTrueMargin),
      m_innerCheckedFalseMargin(innerCheckedFalseMargin)
{
}

TickReader::~TickReader()
{
}
