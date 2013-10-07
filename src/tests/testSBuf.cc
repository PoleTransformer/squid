#include "squid.h"
#include "Mem.h"
#include "SBuf.h"
#include "SBufStream.h"
#include "SquidString.h"
#include "testSBuf.h"
#include "SBufFindTest.h"

#include <iostream>
#include <stdexcept>

CPPUNIT_TEST_SUITE_REGISTRATION( testSBuf );

/* let this test link sanely */
#include "event.h"
#include "MemObject.h"
void
eventAdd(const char *name, EVH * func, void *arg, double when, int, bool cbdata)
{}
int64_t
MemObject::endOffset() const
{ return 0; }
/* end of stubs */

// test string
static char fox[]="The quick brown fox jumped over the lazy dog";
static char fox1[]="The quick brown fox ";
static char fox2[]="jumped over the lazy dog";

// TEST: globals variables (default/empty and with contents) are
//  created outside and before any unit tests and memory subsystem
//  initialization. Check for correct constructor operation.
SBuf empty_sbuf;
SBuf literal("The quick brown fox jumped over the lazy dog");

void
testSBuf::testSBufConstructDestruct()
{
    /* NOTE: Do not initialize memory here because we need
     * to test correct operation before and after Mem::Init
     */

    // XXX: partial demo below of how to do constructor unit-test. use scope to ensure each test
    // is working on local-scope variables constructed fresh for the test, and destructed when
    // scope exists. use nested scopes to test destructor affects on copied data (MemBlob etc)

    // TEST: default constructor (implicit destructor non-crash test)
    //  test accessors on empty SBuf.
    {
        SBuf s1;
        CPPUNIT_ASSERT_EQUAL(0U,s1.length());
        CPPUNIT_ASSERT_EQUAL(SBuf(""),s1);
        CPPUNIT_ASSERT_EQUAL(empty_sbuf,s1);
        CPPUNIT_ASSERT_EQUAL(0,strcmp("",s1.c_str()));
    }

    // TEST: copy-construct NULL string (implicit destructor non-crash test)
    {
        SBuf s1(NULL);
        CPPUNIT_ASSERT_EQUAL(0U,s1.length());
        CPPUNIT_ASSERT_EQUAL(SBuf(""),s1);
        CPPUNIT_ASSERT_EQUAL(empty_sbuf,s1);
        CPPUNIT_ASSERT_EQUAL(0,strcmp("",s1.c_str()));
    }

    // TEST: copy-construct empty string (implicit destructor non-crash test)
    {
        SBuf s1("");
        CPPUNIT_ASSERT_EQUAL(0U,s1.length());
        CPPUNIT_ASSERT_EQUAL(SBuf(""),s1);
        CPPUNIT_ASSERT_EQUAL(empty_sbuf,s1);
        CPPUNIT_ASSERT_EQUAL(0,strcmp("",s1.c_str()));
    }

    // TEST: copy-construct from a SBuf
    {
        SBuf s1(empty_sbuf);
        CPPUNIT_ASSERT_EQUAL(0U,s1.length());
        CPPUNIT_ASSERT_EQUAL(SBuf(""),s1);
        CPPUNIT_ASSERT_EQUAL(empty_sbuf,s1);
        CPPUNIT_ASSERT_EQUAL(0,strcmp("",s1.c_str()));

        SBuf s5(literal);
        CPPUNIT_ASSERT_EQUAL(literal,s5);
        SBuf s6(fox);
        CPPUNIT_ASSERT_EQUAL(literal,s6);
        // XXX: other state checks. expected result of calling any state accessor on s4 ?
    }

    // TEST: check that COW doesn't happen upon copy-construction
    {
        SBuf s1(empty_sbuf), s2(s1);
        CPPUNIT_ASSERT_EQUAL(s1.rawContent(), s2.rawContent());
        SBuf s3(literal), s4(literal);
        CPPUNIT_ASSERT_EQUAL(s3.rawContent(), s4.rawContent());
    }

    // TEST: sub-string copy
    {
        SBuf s1=SBuf(fox+4), s2(fox);
        SBuf s3=s2.substr(4,s2.length()); //n is out-of-bounds
        CPPUNIT_ASSERT_EQUAL(s1,s3);
        SBuf s4=SBuf(fox,4);
        s3=s2.substr(0,4);
        CPPUNIT_ASSERT_EQUAL(s4,s3);
    }

    // TEST: go via SquidString adapters.
    {
        String str(fox);
        SBuf s1(str);
        CPPUNIT_ASSERT_EQUAL(literal,s1);
    }

    // TEST: go via std::string adapter.
    {
        std::string str(fox);
        SBuf s1(str);
        CPPUNIT_ASSERT_EQUAL(literal,s1);
    }
}

void
testSBuf::testSBufConstructDestructAfterMemInit()
{
    Mem::Init();
    testSBufConstructDestruct();
}

void
testSBuf::testEqualityTest()
{
    SBuf s1(fox),s2(fox);
    CPPUNIT_ASSERT_EQUAL(s1,s1); //self-equality
    CPPUNIT_ASSERT_EQUAL(s1,s2); //same contents
    s2.assign("The quick brown fox jumped over the lazy doe");
    CPPUNIT_ASSERT(!(s1 == s2)); //same length, different contents
    s2.assign("foo");
    CPPUNIT_ASSERT(!(s1 == s2)); //different length and contents
    CPPUNIT_ASSERT(s1 != s2);    //while we're ready, let's test inequality
    s2.clear();
    CPPUNIT_ASSERT(!(s1 == s2)); //null and not-null
    CPPUNIT_ASSERT(s1 != s2);    //while we're ready, let's test inequality
    s1.clear();
    CPPUNIT_ASSERT_EQUAL(s1,s2); //null and null
}

void
testSBuf::testAppendSBuf()
{
    SBuf s1(fox1),s2(fox2);
    s1.append(s2);
    CPPUNIT_ASSERT_EQUAL(s1,literal);
}

void
testSBuf::testPrintf()
{
    SBuf s1,s2;
    s1.Printf("%s:%d:%03.3f","fox",10,12345.67);
    s2.assign("fox:10:12345.670");
    CPPUNIT_ASSERT_EQUAL(s1,s2);
}

void
testSBuf::testAppendCString()
{
    SBuf s1(fox1);
    s1.append(fox2);
    CPPUNIT_ASSERT_EQUAL(s1,literal);
}

void
testSBuf::testAppendStdString()
{
    const char *alphabet="abcdefghijklmnopqrstuvwxyz";
    {
        SBuf alpha(alphabet), s;
        s.append(alphabet,5).append(alphabet+5);
        CPPUNIT_ASSERT_EQUAL(alpha,s);
    }
    {
        SBuf s;
        std::string control;
        s.append(alphabet,5).append("\0",1).append(alphabet+6,SBuf::npos);
        control.append(alphabet,5).append(1,'\0').append(alphabet,6,std::string::npos);
        SBuf scontrol(control); // we need this to test the equality. sigh.
        CPPUNIT_ASSERT_EQUAL(scontrol,s);
    }
    {
        const char *alphazero="abcdefghijk\0mnopqrstuvwxyz";
        SBuf s(alphazero,26);
        std::string str(alphazero,26);
        CPPUNIT_ASSERT_EQUAL(0,memcmp(str.data(),s.rawContent(),26));
    }
}

void
testSBuf::testAppendf()
{
    SBuf s1,s2;
    s1.appendf("%s:%d:%03.2f",fox,1234,1234.56);
    s2.assign("The quick brown fox jumped over the lazy dog:1234:1234.56");
    CPPUNIT_ASSERT_EQUAL(s2,s1);
}

void
testSBuf::testDumpStats()
{
    SBuf::GetStats().dump(std::cout);
    MemBlob::GetStats().dump(std::cout);
    std::cout << "sizeof(SBuf): " << sizeof(SBuf) << std::endl;
    std::cout << "sizeof(MemBlob): " << sizeof(MemBlob) << std::endl;
}

void
testSBuf::testSubscriptOp()
{
    SBuf chg(literal);
    CPPUNIT_ASSERT_EQUAL(chg[5],'u');
    chg.setAt(5,'e');
    CPPUNIT_ASSERT_EQUAL(literal[5],'u');
    CPPUNIT_ASSERT_EQUAL(chg[5],'e');
}

// note: can't use cppunit's CPPUNIT_TEST_EXCEPTION because TextException asserts, and
// so the test can't be properly completed.
void
testSBuf::testSubscriptOpFail()
{
    char c;
    c=literal.at(literal.length()); //out of bounds by 1
    //notreached
    std::cout << c << std::endl;
}

static int sign(int v)
{
    if (v < 0)
        return -1;
    if (v>0)
        return 1;
    return 0;
}

void
testSBuf::testComparisons()
{
    //same length
    SBuf s1("foo"),s2("foe");
    CPPUNIT_ASSERT(s1.cmp(s2)>0);
    CPPUNIT_ASSERT(s1.caseCmp(s2)>0);
    CPPUNIT_ASSERT(s2.cmp(s1)<0);
    CPPUNIT_ASSERT_EQUAL(0,s1.cmp(s2,2));
    CPPUNIT_ASSERT_EQUAL(0,s1.caseCmp(s2,2));
    CPPUNIT_ASSERT(s1 > s2);
    CPPUNIT_ASSERT(s2 < s1);
    CPPUNIT_ASSERT_EQUAL(sign(s1.cmp(s2)),sign(strcmp(s1.c_str(),s2.c_str())));
    //different lengths
    s1.assign("foo");
    s2.assign("foof");
    CPPUNIT_ASSERT(s1.cmp(s2)<0);
    CPPUNIT_ASSERT_EQUAL(sign(s1.cmp(s2)),sign(strcmp(s1.c_str(),s2.c_str())));
    CPPUNIT_ASSERT(s1 < s2);
    // specifying the max-length and overhanging size
    CPPUNIT_ASSERT_EQUAL(1,SBuf("foolong").caseCmp(SBuf("foo"), 5));
    // case-insensive comaprison
    s1 = "foo";
    s2 = "fOo";
    CPPUNIT_ASSERT_EQUAL(0,s1.caseCmp(s2));
    CPPUNIT_ASSERT_EQUAL(0,s1.caseCmp(s2,2));
    // \0-clenliness test
    s1.assign("f\0oo",4);
    s2.assign("f\0Oo",4);
    CPPUNIT_ASSERT_EQUAL(1,s1.cmp(s2));
    CPPUNIT_ASSERT_EQUAL(0,s1.caseCmp(s2));
    CPPUNIT_ASSERT_EQUAL(0,s1.caseCmp(s2,3));
    CPPUNIT_ASSERT_EQUAL(0,s1.caseCmp(s2,2));
    CPPUNIT_ASSERT_EQUAL(0,s1.cmp(s2,2));
}

void
testSBuf::testConsume()
{
    SBuf s1(literal),s2,s3;
    s2=s1.consume(4);
    s3.assign("The ");
    CPPUNIT_ASSERT_EQUAL(s2,s3);
    s3.assign("quick brown fox jumped over the lazy dog");
    CPPUNIT_ASSERT_EQUAL(s1,s3);
    s1.consume(40);
    CPPUNIT_ASSERT_EQUAL(s1,SBuf());
}

void
testSBuf::testRawContent()
{
    SBuf s1(literal);
    SBuf s2(s1);
    s2.append("foo");
    const char *foo;
    foo = s1.rawContent();
    CPPUNIT_ASSERT_EQUAL(0,strncmp(fox,foo,s1.length()));
    foo = s1.c_str();
    CPPUNIT_ASSERT(!strcmp(fox,foo));
}

void
testSBuf::testRawSpace()
{
    SBuf s1(literal);
    SBuf s2(fox1);
    SBuf::size_type sz=s2.length();
    char *rb=s2.rawSpace(strlen(fox2)+1);
    strcpy(rb,fox2);
    s2.forceSize(sz+strlen(fox2));
    CPPUNIT_ASSERT_EQUAL(s1,s2);
}

void
testSBuf::testChop()
{
    SBuf s1(literal),s2;
    s1.chop(4,5);
    s2.assign("quick");
    CPPUNIT_ASSERT_EQUAL(s1,s2);
    s1=literal;
    s2.clear();
    s1.chop(5,0);
    CPPUNIT_ASSERT_EQUAL(s1,s2);
    const char *alphabet="abcdefghijklmnopqrstuvwxyz";
    SBuf a(alphabet);
    std::string s(alphabet); // TODO
    { //regular chopping
        SBuf b(a);
        b.chop(3,3);
        SBuf ref("def");
        CPPUNIT_ASSERT_EQUAL(ref,b);
    }
    { // chop at end
        SBuf b(a);
        b.chop(b.length()-3);
        SBuf ref("xyz");
        CPPUNIT_ASSERT_EQUAL(ref,b);
    }
    { // chop at beginning
        SBuf b(a);
        b.chop(0,3);
        SBuf ref("abc");
        CPPUNIT_ASSERT_EQUAL(ref,b);
    }
    { // chop to zero length
        SBuf b(a);
        b.chop(5,0);
        SBuf ref("");
        CPPUNIT_ASSERT_EQUAL(ref,b);
    }
    { // chop beyond end (at npos)
        SBuf b(a);
        b.chop(SBuf::npos,4);
        SBuf ref("");
        CPPUNIT_ASSERT_EQUAL(ref,b);
    }
    { // chop beyond end
        SBuf b(a);
        b.chop(b.length()+2,4);
        SBuf ref("");
        CPPUNIT_ASSERT_EQUAL(ref,b);
    }
    { // null-chop
        SBuf b(a);
        b.chop(0,b.length());
        SBuf ref(a);
        CPPUNIT_ASSERT_EQUAL(ref,b);
    }
    { // overflow chopped area
        SBuf b(a);
        b.chop(b.length()-3,b.length());
        SBuf ref("xyz");
        CPPUNIT_ASSERT_EQUAL(ref,b);
    }
}

void
testSBuf::testChomp()
{
    SBuf s1("complete string");
    SBuf s2(s1);
    s2.trim(SBuf(" ,"));
    CPPUNIT_ASSERT_EQUAL(s1,s2);
    s2.assign(" complete string ,");
    s2.trim(SBuf(" ,"));
    CPPUNIT_ASSERT_EQUAL(s1,s2);
    s1.assign(", complete string ,");
    s2=s1;
    s2.trim(SBuf(" "));
    CPPUNIT_ASSERT_EQUAL(s1,s2);
}

// inspired by SBufFindTest; to be expanded.
class SBufSubstrAutoTest
{
    SBuf fullString, sb;
    std::string fullReference, str;
public:
    void performEqualityTest() {
        SBuf ref(str);
        CPPUNIT_ASSERT_EQUAL(ref,sb);
    }
    SBufSubstrAutoTest() : fullString(fox), fullReference(fox) {
        for (int offset=fullString.length()-1; offset >= 0; --offset ) {
            for (int length=fullString.length()-1-offset; length >= 0; --length) {
                sb=fullString.substr(offset,length);
                str=fullReference.substr(offset,length);
                performEqualityTest();
            }
        }
    }
};

void
testSBuf::testSubstr()
{
    SBuf s1(literal),s2,s3;
    s2=s1.substr(4,5);
    s3.assign("quick");
    CPPUNIT_ASSERT_EQUAL(s2,s3);
    s1.chop(4,5);
    CPPUNIT_ASSERT_EQUAL(s1,s2);
    SBufSubstrAutoTest sat; // work done in the constructor
}

void
testSBuf::testFindChar()
{
    const char *alphabet="abcdefghijklmnopqrstuvwxyz";
    SBuf s1(alphabet);
    SBuf::size_type idx;
    SBuf::size_type nposResult=SBuf::npos;

    // FORWARD SEARCH
    // needle in haystack
    idx=s1.find('d');
    CPPUNIT_ASSERT_EQUAL(3U,idx);
    CPPUNIT_ASSERT_EQUAL('d',s1[idx]);

    // needle not present in haystack
    idx=s1.find(' '); //fails
    CPPUNIT_ASSERT_EQUAL(nposResult,idx);

    // search in portion
    idx=s1.find('e',3U);
    CPPUNIT_ASSERT_EQUAL(4U,idx);

    // char not in searched portion
    idx=s1.find('e',5U);
    CPPUNIT_ASSERT_EQUAL(nposResult,idx);

    // invalid start position
    idx=s1.find('d',SBuf::npos);
    CPPUNIT_ASSERT_EQUAL(nposResult,idx);

    // search outside of haystack
    idx=s1.find('d',s1.length()+1);
    CPPUNIT_ASSERT_EQUAL(nposResult,idx);

    // REVERSE SEARCH
    // needle in haystack
    idx=s1.rfind('d');
    CPPUNIT_ASSERT_EQUAL(3U, idx);
    CPPUNIT_ASSERT_EQUAL('d', s1[idx]);

    // needle not present in haystack
    idx=s1.rfind(' '); //fails
    CPPUNIT_ASSERT_EQUAL(nposResult,idx);

    // search in portion
    idx=s1.rfind('e',5);
    CPPUNIT_ASSERT_EQUAL(4U,idx);

    // char not in searched portion
    idx=s1.rfind('e',3);
    CPPUNIT_ASSERT_EQUAL(nposResult,idx);

    // overlong haystack specification
    idx=s1.rfind('d',s1.length()+1);
    CPPUNIT_ASSERT_EQUAL(3U,idx);
}

void
testSBuf::testFindSBuf()
{
    const char *alphabet="abcdefghijklmnopqrstuvwxyz";
    SBuf haystack(alphabet);
    SBuf::size_type idx;
    SBuf::size_type nposResult=SBuf::npos;

    // FORWARD search
    // needle in haystack
    idx = haystack.find(SBuf("def"));
    CPPUNIT_ASSERT_EQUAL(3U,idx);

    idx = haystack.find(SBuf("xyz"));
    CPPUNIT_ASSERT_EQUAL(23U,idx);

    // needle not in haystack, no initial char match
    idx = haystack.find(SBuf(" eq"));
    CPPUNIT_ASSERT_EQUAL(nposResult, idx);

    // needle not in haystack, initial sequence match
    idx = haystack.find(SBuf("deg"));
    CPPUNIT_ASSERT_EQUAL(nposResult, idx);

    // needle past end of haystack
    idx = haystack.find(SBuf("xyz1"));
    CPPUNIT_ASSERT_EQUAL(nposResult, idx);

    // search in portion: needle not in searched part
    idx = haystack.find(SBuf("def"),7);
    CPPUNIT_ASSERT_EQUAL(nposResult, idx);

    // search in portion: overhang
    idx = haystack.find(SBuf("def"),4);
    CPPUNIT_ASSERT_EQUAL(nposResult, idx);

    // invalid start position
    idx = haystack.find(SBuf("def"),SBuf::npos);
    CPPUNIT_ASSERT_EQUAL(nposResult, idx);

    // needle bigger than haystack
    idx = SBuf("def").find(haystack);
    CPPUNIT_ASSERT_EQUAL(nposResult, idx);

    // search in a double-matching haystack
    {
        SBuf h2=haystack;
        h2.append(haystack);

        idx = h2.find(SBuf("def"));
        CPPUNIT_ASSERT_EQUAL(3U,idx);

        idx = h2.find(SBuf("xyzab"));
        CPPUNIT_ASSERT_EQUAL(23U,idx);
    }

    // REVERSE search
    // needle in haystack
    idx = haystack.rfind(SBuf("def"));
    CPPUNIT_ASSERT_EQUAL(3U,idx);

    idx = haystack.rfind(SBuf("xyz"));
    CPPUNIT_ASSERT_EQUAL(23U,idx);

    // needle not in haystack, no initial char match
    idx = haystack.rfind(SBuf(" eq"));
    CPPUNIT_ASSERT_EQUAL(nposResult, idx);

    // needle not in haystack, initial sequence match
    idx = haystack.rfind(SBuf("deg"));
    CPPUNIT_ASSERT_EQUAL(nposResult, idx);

    // needle past end of haystack
    idx = haystack.rfind(SBuf("xyz1"));
    CPPUNIT_ASSERT_EQUAL(nposResult, idx);

    // search in portion: needle in searched part
    idx = haystack.rfind(SBuf("def"),7);
    CPPUNIT_ASSERT_EQUAL(3U, idx);

    // search in portion: needle not in searched part
    idx = haystack.rfind(SBuf("mno"),3);
    CPPUNIT_ASSERT_EQUAL(nposResult, idx);

    // search in portion: overhang
    idx = haystack.rfind(SBuf("def"),4);
    CPPUNIT_ASSERT_EQUAL(3U, idx);

    // npos start position
    idx = haystack.rfind(SBuf("def"),SBuf::npos);
    CPPUNIT_ASSERT_EQUAL(3U, idx);

    // needle bigger than haystack
    idx = SBuf("def").rfind(haystack);
    CPPUNIT_ASSERT_EQUAL(nposResult, idx);

    // search in a double-matching haystack
    {
        SBuf h2=haystack;
        h2.append(haystack);

        idx = h2.rfind(SBuf("def"));
        CPPUNIT_ASSERT_EQUAL(29U,idx);

        idx = h2.find(SBuf("xyzab"));
        CPPUNIT_ASSERT_EQUAL(23U,idx);
    }
}

void
testSBuf::testRFindChar()
{
    SBuf s1(literal);
    SBuf::size_type idx;
    idx=s1.rfind(' ');
    CPPUNIT_ASSERT_EQUAL(40U,idx);
    CPPUNIT_ASSERT_EQUAL(' ',s1[idx]);
}

void
testSBuf::testRFindSBuf()
{
    SBuf haystack(literal),afox("fox");
    SBuf goobar("goobar");
    SBuf::size_type idx;

    // corner case: search for a zero-length SBuf
    idx=haystack.rfind(SBuf(""));
    CPPUNIT_ASSERT_EQUAL(haystack.length(),idx);

    // corner case: search for a needle longer than the haystack
    idx=afox.rfind(SBuf("     "));
    CPPUNIT_ASSERT_EQUAL(SBuf::npos,idx);

    idx=haystack.rfind(SBuf("fox"));
    CPPUNIT_ASSERT_EQUAL(16U,idx);

    // needle not found, no match for first char
    idx=goobar.rfind(SBuf("foo"));
    CPPUNIT_ASSERT_EQUAL(SBuf::npos,idx);

    // needle not found, match for first char but no match for SBuf
    idx=haystack.rfind(SBuf("foe"));
    CPPUNIT_ASSERT_EQUAL(SBuf::npos,idx);

    SBuf g("g"); //match at the last char
    idx=haystack.rfind(g);
    CPPUNIT_ASSERT_EQUAL(43U,idx);
    CPPUNIT_ASSERT_EQUAL('g',haystack[idx]);

    idx=haystack.rfind(SBuf("The"));
    CPPUNIT_ASSERT_EQUAL(0U,idx);

    haystack.append("The");
    idx=haystack.rfind(SBuf("The"));
    CPPUNIT_ASSERT_EQUAL(44U,idx);

    //partial match
    haystack="The quick brown fox";
    SBuf needle("foxy lady");
    idx=haystack.rfind(needle);
    CPPUNIT_ASSERT_EQUAL(SBuf::npos,idx);
}

void
testSBuf::testSBufLength()
{
    SBuf s(fox);
    CPPUNIT_ASSERT_EQUAL(strlen(fox),(size_t)s.length());
}

void
testSBuf::testScanf()
{
    SBuf s1;
    char s[128];
    int i;
    float f;
    int rv;
    s1.assign("string , 123 , 123.50");
    rv=s1.scanf("%s , %d , %f",s,&i,&f);
    CPPUNIT_ASSERT_EQUAL(3,rv);
    CPPUNIT_ASSERT_EQUAL(0,strcmp(s,"string"));
    CPPUNIT_ASSERT_EQUAL(123,i);
    CPPUNIT_ASSERT_EQUAL(static_cast<float>(123.5),f);
}

void testSBuf::testCopy()
{
    char buf[40]; //shorter than literal()
    SBuf s(fox1),s2;
    CPPUNIT_ASSERT_EQUAL(s.length(),s.copy(buf,40));
    CPPUNIT_ASSERT_EQUAL(0,strncmp(s.rawContent(),buf,s.length()));
    s=literal;
    CPPUNIT_ASSERT_EQUAL(40U,s.copy(buf,40));
    s2.assign(buf,40);
    s.chop(0,40);
    CPPUNIT_ASSERT_EQUAL(s2,s);
}

void testSBuf::testStringOps()
{
    SBuf sng(literal.toLower()),
    ref("the quick brown fox jumped over the lazy dog");
    CPPUNIT_ASSERT_EQUAL(ref,sng);
    sng=literal;
    CPPUNIT_ASSERT_EQUAL(0,sng.compare(ref,caseInsensitive));
    // max-size comparison
    CPPUNIT_ASSERT_EQUAL(0,ref.compare(SBuf("THE"),caseInsensitive,3));
    CPPUNIT_ASSERT_EQUAL(1,ref.compare(SBuf("THE"),caseInsensitive,6));
    CPPUNIT_ASSERT_EQUAL(0,SBuf("the").compare(SBuf("THE"),caseInsensitive,6));
}

void testSBuf::testGrow()
{
    SBuf t;
    t.assign("foo");
    const char *ref=t.rawContent();
    t.reserveCapacity(10240);
    const char *match=t.rawContent();
    CPPUNIT_ASSERT(match!=ref);
    ref=match;
    t.append(literal).append(literal).append(literal).append(literal).append(literal);
    t.append(t).append(t).append(t).append(t).append(t);
    CPPUNIT_ASSERT_EQUAL(ref,match);
}

void testSBuf::testStartsWith()
{
    static SBuf casebuf("THE QUICK");
    CPPUNIT_ASSERT(literal.startsWith(SBuf(fox1)));
    CPPUNIT_ASSERT(!SBuf("The quick brown").startsWith(SBuf(fox1))); //too short
    CPPUNIT_ASSERT(!literal.startsWith(SBuf(fox2))); //different contents

    // case-insensitive checks
    CPPUNIT_ASSERT(literal.startsWith(casebuf,caseInsensitive));
    casebuf=SBuf(fox1).toUpper();
    CPPUNIT_ASSERT(literal.startsWith(casebuf,caseInsensitive));
    CPPUNIT_ASSERT(literal.startsWith(SBuf(fox1),caseInsensitive));
    casebuf = "tha quick";
    CPPUNIT_ASSERT_EQUAL(false,literal.startsWith(casebuf,caseInsensitive));
}

void testSBuf::testSBufStream()
{
    SBuf b("const.string, int 10 and a float 10.5");
    SBufStream ss;
    ss << "const.string, int " << 10 << " and a float " << 10.5;
    SBuf o=ss.buf();
    CPPUNIT_ASSERT_EQUAL(b,o);
    ss.clearBuf();
    o=ss.buf();
    CPPUNIT_ASSERT_EQUAL(SBuf(),o);
    SBuf f1(fox1);
    SBufStream ss2(f1);
    ss2 << fox2;
    CPPUNIT_ASSERT_EQUAL(ss2.buf(),literal);
    CPPUNIT_ASSERT_EQUAL(f1,SBuf(fox1));
}

void testSBuf::testFindFirstOf()
{
    SBuf haystack(literal);
    SBuf::size_type idx;

    // not found
    idx=haystack.find_first_of(SBuf("ADHRWYP"));
    CPPUNIT_ASSERT_EQUAL(SBuf::npos,idx);

    // found at beginning
    idx=haystack.find_first_of(SBuf("THANDF"));
    CPPUNIT_ASSERT_EQUAL(0U,idx);

    //found at end of haystack
    idx=haystack.find_first_of(SBuf("QWERYVg"));
    CPPUNIT_ASSERT_EQUAL(haystack.length()-1,idx);

    //found in the middle of haystack
    idx=haystack.find_first_of(SBuf("QWERqYV"));
    CPPUNIT_ASSERT_EQUAL(4U,idx);
}

void testSBuf::testAutoFind()
{
    SBufFindTest test;
    test.run();
}

void testSBuf::testStdStringOps()
{
    const char *alphabet="abcdefghijklmnopqrstuvwxyz";
    std::string astr(alphabet);
    SBuf sb(alphabet);
    CPPUNIT_ASSERT_EQUAL(astr,sb.toStdString());
}