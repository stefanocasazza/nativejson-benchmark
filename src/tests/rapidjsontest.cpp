#include "../test.h"

// Turn on SSE2 for x86-64.
#if defined(_M_X64) || defined(__amd64)
#define RAPIDJSON_SSE2
#endif

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

using namespace rapidjson;

#if SLOWER_STAT
class StatHandler {
public:
    typedef char Ch;

    StatHandler(Stat& stat) : stat_(stat) {}

    bool Null() { stat_.nullCount++; return true;  }
    bool Bool(bool b) { if (b) stat_.trueCount++; else stat_.falseCount++; return true; }
    bool Int(int) { stat_.numberCount++; return true; }
    bool Uint(unsigned) { stat_.numberCount++; return true; }
    bool Int64(int64_t) { stat_.numberCount++; return true; }
    bool Uint64(uint64_t) { stat_.numberCount++; return true; }
    bool Double(double) { stat_.numberCount++; return true; }
    bool String(const Ch*, SizeType length, bool) { stat_.stringCount++; stat_.stringLength += length; return true; }
    bool StartObject() { return true; }
    bool EndObject(SizeType memberCount) { stat_.objectCount++; stat_.memberCount += memberCount; return true; }
    bool StartArray() { return true; }
    bool EndArray(SizeType elementCount) { stat_.arrayCount++; stat_.elementCount += elementCount; return true; }

private:
    StatHandler& operator=(const StatHandler&);

    Stat& stat_;
};
#else
static void GenStat(Stat& stat, const Value& v) {
    switch(v.GetType()) {
    case kNullType:  stat.nullCount++; break;
    case kFalseType: stat.falseCount++; break;
    case kTrueType:  stat.trueCount++; break;

    case kObjectType:
        for (Value::ConstMemberIterator m = v.MemberBegin(); m != v.MemberEnd(); ++m) {
            stat.stringLength += m->name.GetStringLength();
            GenStat(stat, m->value);
        }
        stat.objectCount++;
        stat.memberCount += (v.MemberEnd() - v.MemberBegin());
        stat.stringCount += (v.MemberEnd() - v.MemberBegin()); // Key
        break;

    case kArrayType:
        for (Value::ConstValueIterator i = v.Begin(); i != v.End(); ++i)
            GenStat(stat, *i);
        stat.arrayCount++;
        stat.elementCount += v.Size();
        break;

    case kStringType:
        stat.stringCount++;
        stat.stringLength += v.GetStringLength();
        break;

    case kNumberType:
        stat.numberCount++;
        break;
    }
}
#endif

class RapidjsonParseResult : public ParseResultBase {
public:
    Document document;
};

class RapidjsonStringResult : public StringResultBase {
public:
    virtual const char* c_str() const { return sb.GetString(); }

    StringBuffer sb;
};

class RapidjsonTest : public TestBase {
public:
	RapidjsonTest() : TestBase("RapidJSON") {
	}
	
    virtual ParseResultBase* Parse(const char* json, size_t length) const {
        (void)length;
        RapidjsonParseResult* pr = new RapidjsonParseResult;
    	if (pr->document.Parse(json).HasParseError()) {
            delete pr;
    		return 0;
    	}
    	return pr;
    }

    virtual StringResultBase* Stringify(const ParseResultBase* parseResult) const {
        const RapidjsonParseResult* pr = static_cast<const RapidjsonParseResult*>(parseResult);
        RapidjsonStringResult* sr = new RapidjsonStringResult;
    	Writer<StringBuffer> writer(sr->sb);
    	pr->document.Accept(writer);
    	return sr;
    }

    virtual StringResultBase* Prettify(const ParseResultBase* parseResult) const {
        const RapidjsonParseResult* pr = static_cast<const RapidjsonParseResult*>(parseResult);
        RapidjsonStringResult* sr = new RapidjsonStringResult;
    	PrettyWriter<StringBuffer> writer(sr->sb);
        pr->document.Accept(writer);
        return sr;
    }

    virtual bool Statistics(const ParseResultBase* parseResult, Stat* stat) const {
        const RapidjsonParseResult* pr = static_cast<const RapidjsonParseResult*>(parseResult);
        memset(stat, 0, sizeof(Stat));
#if SLOWER_STAT
        StatHandler h(*stat);
        doc->Accept(h);
#else
        GenStat(*stat, pr->document);
#endif
        return true;
    }
};

REGISTER_TEST(RapidjsonTest);
