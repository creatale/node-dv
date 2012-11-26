#ifndef OMR_H
#define OMR_H

#include <v8.h>
#include <node.h>

class OMR : public node::ObjectWrap
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

    OMR(double outerCheckedThreshold, double outerCheckedTrueMargin, double outerCheckedFalseMargin, double innerCheckedThreshold, double innerCheckedTrueMargin, double innerCheckedFalseMargin);
    ~OMR();

    double m_outerCheckedThreshold;
    double m_outerCheckedTrueMargin;
    double m_outerCheckedFalseMargin;
    double m_innerCheckedThreshold;
    double m_innerCheckedTrueMargin;
    double m_innerCheckedFalseMargin;
};

#endif
