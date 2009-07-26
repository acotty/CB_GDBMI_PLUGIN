#include <tr1/memory>
#include <UnitTest++.h>

#include "cmd_result_parser.h"
#include "cmd_result_tokens.h"

namespace dbg_mi
{
    extern wxString TrimmedSubString(wxString const &str, int start, int length);
}
TEST(TrimmedSubString)
{
     wxString str = dbg_mi::TrimmedSubString(_T("asd as    df"), 4, 3);

     CHECK(str == _T("as"));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<int N>
bool TestGetNextToken(wxString const &s, dbg_mi::Token &t)
{
    int pos = 0;
    for(int ii = 0; ii < N; ++ii)
    {
        if(!dbg_mi::GetNextToken(s, pos, t))
            return false;

        pos = t.end;
    }
    return true;
};

TEST(TestGetNextToken1)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<1>(_T("a = 5, b = 6"), t);

    CHECK(r && t.start == 0 && t.end == 1 && t.type == dbg_mi::Token::String);
}
TEST(TestGetNextToken2)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<2>(_T("a = 5, b = 6"), t);

    CHECK(r && t.start == 2 && t.end == 3 && t.type == dbg_mi::Token::Equal);
}
TEST(TestGetNextToken3)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<3>(_T("ab = 5, b = 6"), t);

    CHECK(r && t.start == 5 && t.end == 6 && t.type == dbg_mi::Token::String);
}
TEST(TestGetNextToken4)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<4>(_T("a = 5, b = 6"), t);

    CHECK(r && t.start == 5 && t.end == 6 && t.type == dbg_mi::Token::Comma);
}
TEST(TestGetNextToken5)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<5>(_T("a = 5, bdb=\"str\""), t);

    CHECK(r && t.start == 7 && t.end == 10 && t.type == dbg_mi::Token::String);
}
TEST(TestGetNextToken6)
{
    dbg_mi::Token t;
    wxString s = _T("a = 5, bdb=\"str\"");
    bool r = TestGetNextToken<7>(s, t);

    CHECK(r && t.start == 11 && t.end == 16 && t.type == dbg_mi::Token::String);
}
TEST(TestGetNextToken7)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<3>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t.start == 4 && t.end == 5 && t.type == dbg_mi::Token::ListStart);
}
TEST(TestGetNextToken8)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<4>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t == dbg_mi::Token(5, 6, dbg_mi::Token::String));
}
TEST(TestGetNextToken9)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<5>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t == dbg_mi::Token(6, 7, dbg_mi::Token::Comma));
}
TEST(TestGetNextToken10)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<6>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t == dbg_mi::Token(7, 13, dbg_mi::Token::String));
}
TEST(TestGetNextToken11)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<9>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t == dbg_mi::Token(16, 17, dbg_mi::Token::ListEnd));
}
TEST(TestGetNextToken12)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<10>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t == dbg_mi::Token(17, 18, dbg_mi::Token::Comma));
}
TEST(TestGetNextToken13)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<13>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t == dbg_mi::Token(22, 23, dbg_mi::Token::TupleStart));
}
TEST(TestGetNextToken14)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<21>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t == dbg_mi::Token(39, 40, dbg_mi::Token::TupleEnd));
}

TEST(TestGetNextToken15)
{
    dbg_mi::Token t;
    wxString s = _T("a = \"-\\\"ast\\\"-\"");
    bool r = TestGetNextToken<3>(s, t);
    CHECK(r && t == dbg_mi::Token(4, 15, dbg_mi::Token::String));
    CHECK(r && t.ExtractString(s) == _T("\"-\\\"ast\\\"-\""));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(ResultValueMakeDebugString_Simple)
{
    dbg_mi::ResultValue r(_T("name"), dbg_mi::ResultValue::Simple);
    r.SetSimpleValue(_T("value"));

    CHECK(r.MakeDebugString() == _T("name=value"));
}
TEST(ResultValueMakeDebugString_Tuple)
{
    dbg_mi::ResultValue r(_T("name"), dbg_mi::ResultValue::Tuple);

    dbg_mi::ResultValue *a = new dbg_mi::ResultValue(_T("name1"), dbg_mi::ResultValue::Simple);
    a->SetSimpleValue(_T("value1"));

    dbg_mi::ResultValue *b = new dbg_mi::ResultValue(_T("name2"), dbg_mi::ResultValue::Simple);
    b->SetSimpleValue(_T("value2"));

    r.SetTupleValue(a);
    r.SetTupleValue(b);

    wxString s = r.MakeDebugString();
    CHECK(s == _T("name={name1=value1,name2=value2}"));
}
TEST(ResultValueMakeDebugString3)
{
    dbg_mi::ResultValue r(_T("name"), dbg_mi::ResultValue::Array);

    dbg_mi::ResultValue *a = new dbg_mi::ResultValue(_T(""), dbg_mi::ResultValue::Simple);
    a->SetSimpleValue(_T("1"));

    dbg_mi::ResultValue *b = new dbg_mi::ResultValue(_T(""), dbg_mi::ResultValue::Simple);
    b->SetSimpleValue(_T("2"));

    dbg_mi::ResultValue *c = new dbg_mi::ResultValue(_T(""), dbg_mi::ResultValue::Simple);
    c->SetSimpleValue(_T("3"));

    r.SetTupleValue(a);
    r.SetTupleValue(b);
    r.SetTupleValue(c);

    wxString s = r.MakeDebugString();
    CHECK(s == _T("name=[1,2,3]"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct TestSimpleValue
{
    TestSimpleValue()
    {
        status = dbg_mi::ParseValue(_T("a = 5"), results);

        result = results.GetTupleValue(_T("a"));
    }

    dbg_mi::ResultValue results;
    dbg_mi::ResultValue const *result;
    bool status;
};

TEST_FIXTURE(TestSimpleValue, Status)
{
    CHECK(status);
}

TEST_FIXTURE(TestSimpleValue, ResultValue_SimpleValueType)
{
    CHECK(result && result->GetType() == dbg_mi::ResultValue::Simple);
}
TEST_FIXTURE(TestSimpleValue, ResultValue_SimpleName)
{
    CHECK(result && result->GetName() == _T("a"));
}

TEST_FIXTURE(TestSimpleValue, ResultValue_SimpleValue)
{
    CHECK(result && result->GetSimpleValue() == _T("5"));
}

TEST_FIXTURE(TestSimpleValue, TestSimple_DebugString)
{
    CHECK(results.MakeDebugString() == _T("{a=5}"));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct TestSimpleValue2
{
    TestSimpleValue2()
    {
        status = dbg_mi::ParseValue(_T("a = 5, b = 6"), result);
//        wprintf(L">%s<\n", result.MakeDebugString().utf8_str().data());

        v_a = result.GetTupleValue(_T("a"));
        v_b = result.GetTupleValue(_T("b"));
    }

    dbg_mi::ResultValue result;
    dbg_mi::ResultValue const *v_a;
    dbg_mi::ResultValue const *v_b;
    bool status;
};


TEST_FIXTURE(TestSimpleValue2, Status)
{
    CHECK(status);
}

TEST_FIXTURE(TestSimpleValue2, Count)
{
    CHECK(result.GetTupleSize() == 2);
}

TEST_FIXTURE(TestSimpleValue2, Values)
{
    CHECK(v_a);
    CHECK(v_b);
}

TEST_FIXTURE(TestSimpleValue2, ResultValue_SimpleValueType1)
{
    CHECK(v_a && v_a->GetType() == dbg_mi::ResultValue::Simple);
}
TEST_FIXTURE(TestSimpleValue2, ResultValue_SimpleValueType2)
{
    CHECK(v_b && v_b->GetType() == dbg_mi::ResultValue::Simple);
}

TEST_FIXTURE(TestSimpleValue2, ResultValue_SimpleValue1)
{
    CHECK(v_a && v_a->GetSimpleValue() == _T("5"));
}

TEST_FIXTURE(TestSimpleValue2, ResultValue_SimpleValue2)
{
    CHECK(v_b && v_b->GetSimpleValue() == _T("6"));
}

TEST_FIXTURE(TestSimpleValue2, TestSimple_DebugString2)
{
    CHECK(result.MakeDebugString() == _T("{a=5,b=6}"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct TestTuple1
{
    TestTuple1()
    {
        status = dbg_mi::ParseValue(_T("a = {b = 5, c = 6}"), result);
        v_a = result.GetTupleValue(_T("a"));
    }

    dbg_mi::ResultValue result;
    dbg_mi::ResultValue const *v_a;
    bool status;
};

TEST_FIXTURE(TestTuple1, Tuple_Status)
{
    CHECK(status);
}

TEST_FIXTURE(TestTuple1, Tuple_DebugString)
{
    wxString s = result.MakeDebugString();
    CHECK(s == _T("{a={b=5,c=6}}"));
}

TEST_FIXTURE(TestTuple1, Tuple_ValueA_Type)
{
    CHECK(v_a && v_a->GetType() == dbg_mi::ResultValue::Tuple);
}

TEST_FIXTURE(TestTuple1, Tuple_ValueA_Value)
{
    dbg_mi::ResultValue const *b = v_a ? v_a->GetTupleValue(_T("b")) : NULL;
    dbg_mi::ResultValue const *c = v_a ? v_a->GetTupleValue(_T("c")) : NULL;

    CHECK(b && b->GetType() == dbg_mi::ResultValue::Simple && b->GetSimpleValue() == _T("5"));
    CHECK(c && c->GetType() == dbg_mi::ResultValue::Simple && c->GetSimpleValue() == _T("6"));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct TestTuple2
{
    TestTuple2()
    {
        status = dbg_mi::ParseValue(_T("a = {b = 5, c = {c1= 1,c2 =2}}, d=6"), result);
        v_a = result.GetTupleValue(_T("a"));
    }

    dbg_mi::ResultValue result;
    dbg_mi::ResultValue const *v_a;
    dbg_mi::ResultValue const *v_d;
    bool status;
};


TEST_FIXTURE(TestTuple2, Tuple_Status)
{
    CHECK(status);
}

TEST_FIXTURE(TestTuple2, Tuple_DebugString)
{
    wxString s = result.MakeDebugString();
    CHECK(s == _T("{a={b=5,c={c1=1,c2=2}},d=6}"));
}

TEST_FIXTURE(TestTuple2, Tuple_ValueB)
{
    dbg_mi::ResultValue const *b = v_a ? v_a->GetTupleValue(_T("b")) : NULL;

    CHECK(b && b->GetType() == dbg_mi::ResultValue::Simple && b->GetSimpleValue() == _T("5"));
}
TEST_FIXTURE(TestTuple2, Tuple_ValueC1)
{
    dbg_mi::ResultValue const *c = v_a ? v_a->GetTupleValue(_T("c")) : NULL;

    CHECK(c && c->GetType() == dbg_mi::ResultValue::Tuple);
}
TEST_FIXTURE(TestTuple2, Tuple_ValueC2)
{
    dbg_mi::ResultValue const *c = v_a ? v_a->GetTupleValue(_T("c")) : NULL;
    dbg_mi::ResultValue const *c1 = c ? c->GetTupleValue(_T("c1")) : NULL;
    dbg_mi::ResultValue const *c2 = c ? c->GetTupleValue(_T("c2")) : NULL;

    CHECK(c1 && c1->GetType() == dbg_mi::ResultValue::Simple && c1->GetSimpleValue() == _T("1"));
    CHECK(c2 && c2->GetType() == dbg_mi::ResultValue::Simple && c2->GetSimpleValue() == _T("2"));
}

TEST_FIXTURE(TestTuple2, Tuple_ValueD)
{
    dbg_mi::ResultValue const *d = result.GetTupleValue(_T("d"));

    CHECK(d && d->GetType() == dbg_mi::ResultValue::Simple && d->GetSimpleValue() == _T("6"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct TestListValue
{
    TestListValue()
    {
        status = dbg_mi::ParseValue(_T("a = [5,6,7]"), result);

//        printf(">%s<\n", result.MakeDebugString().utf8_str().data());
    }

    dbg_mi::ResultValue result;
    bool status;
};
//
//TEST_FIXTURE(TestListValue, ListStatus)
//{
//    CHECK(status);
//    CHECK(result.GetTupleSize() == 3);
//}
//
//TEST_FIXTURE(TestListValue, ListValue1)
//{
//    dbg_mi::ResultValue const *v = result.GetTupleValueByIndex(0);
//    CHECK(v && v->GetSimpleValue() == _T("5"));
//}
//
//TEST_FIXTURE(TestListValue, ListValue2)
//{
//    dbg_mi::ResultValue const *v = result.GetTupleValueByIndex(1);
//    CHECK(v && v->GetSimpleValue() == _T("6"));
//}
//
//TEST_FIXTURE(TestListValue, ListValue3)
//{
//    dbg_mi::ResultValue const *v = result.GetTupleValueByIndex(2);
//    CHECK(v && v->GetSimpleValue() == _T("7"));
//}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(ResultParse_TestType)
{
    dbg_mi::ResultParser parser;
    CHECK(parser.Parse(_T("done,a = 5, b = 6"), dbg_mi::ResultParser::Result));

    CHECK_EQUAL(parser.GetResultType(), dbg_mi::ResultParser::Result);
}

TEST(ResultParse_TestClass)
{
    dbg_mi::ResultParser parser;
    CHECK(parser.Parse(_T("done,a = 5, b = 6"), dbg_mi::ResultParser::Result));

    CHECK(parser.GetResultClass() == dbg_mi::ResultParser::ClassDone);
}
TEST(ResultParse_NoClassComma)
{
    dbg_mi::ResultParser parser;
    bool r = parser.Parse(_T("done a = 5, b = 6"), dbg_mi::ResultParser::Result);

    CHECK(!r);
}

TEST(ResultParse_TestValues)
{
    dbg_mi::ResultParser parser;
    parser.Parse(_T("done,a = 5, b = 6"), dbg_mi::ResultParser::Result);

    dbg_mi::ResultValue::Container const &v = parser.GetResultValues();

    CHECK_EQUAL(v.size(), 2u);
    CHECK(v[0]->raw_value == _T("a = 5"));
    CHECK(v[1]->raw_value == _T(" b = 6"));
}
