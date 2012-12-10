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
#ifndef TICKREADER_H
#define TICKREADER_H

#include <v8.h>
#include <node.h>

class TickReader : public node::ObjectWrap
{
public:
    static void Init(v8::Handle<v8::Object> target);

private:
    static v8::Handle<v8::Value> New(const v8::Arguments& args);
    static v8::Handle<v8::Value> GetHorizontalProjection(const v8::Arguments& args);
    static v8::Handle<v8::Value> GetVerticalProjection(const v8::Arguments& args);
    static v8::Handle<v8::Value> LocatePeaks(const v8::Arguments& args);
    static v8::Handle<v8::Value> GetFill(const v8::Arguments& args);
    static v8::Handle<v8::Value> GetFillRatio(const v8::Arguments& args);
    static v8::Handle<v8::Value> CheckboxIsChecked(const v8::Arguments& args);

    TickReader(double outerCheckedThreshold,
               double outerCheckedTrueMargin, double outerCheckedFalseMargin,
               double innerCheckedThreshold,
               double innerCheckedTrueMargin, double innerCheckedFalseMargin);
    ~TickReader();

    double m_outerCheckedThreshold;
    double m_outerCheckedTrueMargin;
    double m_outerCheckedFalseMargin;
    double m_innerCheckedThreshold;
    double m_innerCheckedTrueMargin;
    double m_innerCheckedFalseMargin;
};

#endif
