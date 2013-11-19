/********************************************************************\

  Name:         mjson.cxx
  Created by:   Konstantin Olchanski

  Contents:     JSON encoder and decoder

  The JSON parser is written to the specifications at:
    http://www.json.org/
    http://www.ietf.org/rfc/rfc4627.txt

\********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "mjson.h"

static const char* skip_spaces(const char* s)
{
   while (1) {
      // per RFC 4627, "Insignificant whitespace"
      switch (*s) {
      default: return s;
      case ' ': s++; break;
      case '\t': s++; break;
      case '\n': s++; break;
      case '\r': s++; break;
      }
   }
   // NOT REACHED
}

static int hexToInt(char c)
{
   if (c == 0)
      return -1;
   if (c >= '0' && c <= '9')
      return c-'0';
   if (c >= 'a' && c <= 'f')
      return c-'a'+10;
   if (c >= 'A' && c <= 'F')
      return c-'A'+10;
   return -1;
}

static int xparse_unicode(const char* s, const char** sout)
{
   int unicode = 0;

   for (int i=0; i<4; i++) {
      int v = hexToInt(*s);
      if (v < 0) {
         *sout = s;
         return -1;
      }
      unicode = unicode*16 + v;
      s++;
   }

   *sout = s;
   return unicode;
}

static std::string xoutput_unicode(int unicode, bool* error)
{
   // see http://en.wikipedia.org/wiki/UTF-8
   if (unicode >= 0 && unicode <= 0x7F) { // 7 bits
      char buf[2];
      buf[0] = unicode & 0x7F;
      buf[1] = 0;
      return buf;
   }

   // FIXME: does this unicode gibberish work right?

   if (unicode >= 0x80 && unicode <= 0x7FF) { // 11 bits
      char buf[3];
      buf[0] = 0x80|0x40|((unicode>>6)&0x1F); // 5 bits
      buf[1] = 0x80|((unicode>>0)&0x3F); // 6 bits
      buf[3] = 0;
      return buf;
   }

   if (unicode >= 0x800 && unicode <= 0xFFFF) { // 16 bits
      char buf[4];
      buf[0] = 0x80|0x40|0x20|((unicode>>12)&0xF); // 4 bits
      buf[1] = 0x80|((unicode>>6)&0x3F); // 6 bits
      buf[2] = 0x80|((unicode>>0)&0x3F); // 6 bits
      buf[3] = 0;
      return buf;
   }

   *error = true;
   return "";
}

static std::string xparse_string(const char* s, const char** sout, bool *error)
{
   //printf("xstring-->%s\n", s);

   std::string v;

   while (1) {
      if (*s == 0) {
         // error
         *sout = s;
         *error = true;
         return "";
      } else if (*s == '\"') {
         // end of string
         *sout = s+1;
         return v;
      } else if (*s == '\\') {
         // escape sequence
         s++;
         //printf("escape %d (%c)\n", *s, *s);
         switch (*s) {
         case 0:
            // maybe error - unexpected end of string
            *sout = s;
            *error = true;
            return v;
         default:
            // error - unknown escape
            *sout = s;
            *error = true;
            return v;
         case '\"': v += '\"'; s++; break;
         case '\\': v += '\\'; s++; break;
         case '/': v += '/';  s++; break;
         case 'b': v += '\b'; s++; break;
         case 'f': v += '\f'; s++; break;
         case 'n': v += '\n'; s++; break;
         case 'r': v += '\r'; s++; break;
         case 't': v += '\t'; s++; break;
         case 'u': {
            s++;
            int unicode = xparse_unicode(s, sout);
            //printf("unicode %d (0x%x), next %c\n", unicode, unicode, **sout);
            if (unicode < 0) {
               // error - bad unicode
               *sout = s;
               *error = true;
               return v;
            }
            v += xoutput_unicode(unicode, error);
            if (*error) {
               // error - bad unicode
               //*sout = s; // stay pointing at the bad unicode
               *error = true;
               return v;
            }
            s = *sout;
            break;
         }
         }
      } else {
         v += *s;
         s++;
      }
   }

   // NOT REACHED
}

static MJsonNode* parse_something(const char* sin, const char* s, const char** sout);

static MJsonNode* parse_array(const char* sin, const char* s, const char** sout)
{
   //printf("array-->%s\n", s);
   MJsonNode *n = MJsonNode::MakeArray();

   s = skip_spaces(s);

   if (*s == ']') {
      // empty array
      *sout = s+1;
      return n;
   }

   while (1) {
      s = skip_spaces(s);
      
      if (*s == 0) {
         *sout = s;
         return MJsonNode::MakeError(n, "unexpected end of string while parsing array", sin, s);
      }

      MJsonNode *p = parse_something(sin, s, sout);
      if (p == NULL) {
         // sout set by parse_something()
         return MJsonNode::MakeError(n, "cannot parse array element", sin, *sout);
      }
      if (p->GetType() == MJSON_ERROR) {
         // sout set by parse_something()
         return MJsonNode::MakeError(n, "error parsing array element", sin, *sout);
      }

      n->AddToArray(p);

      s = skip_spaces(*sout);
      
      if (*s == ']') {
         // end of array
         *sout = s+1;
         return n;
      }

      if (*s == ',') {
         s++;
         continue;
      }

      *sout = s;
      return MJsonNode::MakeError(n, "unexpected char after array element, should be \',\' or \']\'", sin, s);
   }

   // NOT REACHED
}

static MJsonNode* parse_object(const char* sin, const char* s, const char** sout)
{
   //printf("object-->%s\n", s);

   MJsonNode *n = MJsonNode::MakeObject();

   s = skip_spaces(s);

   if (*s == '}') {
      // empty object
      *sout = s+1;
      return n;
   }

   while (1) {
      s = skip_spaces(s);

      //printf("xobject-->%s\n", s);
      
      if (*s == 0) {
         *sout = s;
         return MJsonNode::MakeError(n, "unexpected end of string while parsing object", sin, s);
      } else if (*s != '\"') {
         *sout = s;
         return MJsonNode::MakeError(n, "unexpected char while parsing object, should be \"\"\"", sin, s);
      }

      bool error = false;
      std::string name = xparse_string(s+1, sout, &error);
      if (error || name.length() < 1) {
         // sout set by parse_something()
         return MJsonNode::MakeError(n, "cannot parse name of object element", sin, *sout);
      }

      s = skip_spaces(*sout);

      if (*s == 0) {
         *sout = s;
         return MJsonNode::MakeError(n, "unexpected end of string after name of object element", sin, s);
      } else if (*s != ':') {
         *sout = s;
         return MJsonNode::MakeError(n, "unexpected char after name of object element, should be \":\"", sin, s);
      }

      MJsonNode *p = parse_something(sin, s+1, sout);
      if (p == NULL) {
         // sout set by parse_something()
         return MJsonNode::MakeError(n, "cannot parse object element", sin, *sout);
      }
      if (p->GetType() == MJSON_ERROR) {
         // sout set by parse_something()
         return MJsonNode::MakeError(n, "error parsing object element", sin, *sout);
      }

      n->AddToObject(name.c_str(), p);

      s = skip_spaces(*sout);

      //printf("xobject-->%s\n", s);
      
      if (*s == '}') {
         // end of object
         *sout = s+1;
         return n;
      }

      if (*s == ',') {
         s++;
         continue;
      }

      // error
      *sout = s;
      return MJsonNode::MakeError(n, "unexpected char after object element, should be \"}\" or \",\"", sin, s);
   }

   // NOT REACHED
}

static MJsonNode* parse_string(const char* sin, const char* s, const char** sout)
{
   //printf("string-->%s\n", s);

   bool error = false;
   std::string v = xparse_string(s, sout, &error);

   if (error)
      return MJsonNode::MakeError(NULL, "cannot parse string", sin, *sout);

   return MJsonNode::MakeString(v.c_str());
}

static std::string parse_digits(const char* s, const char** sout)
{
   std::string v;

   while (*s) {
      if (*s < '0')
         break;
      if (*s > '9')
         break;

      v += *s;
      s++;
   }
   
   *sout = s;
   return v;
}

static int patoi_with_overflow(const char* s)
{
   const int kMAX = INT_MAX/10;
   const int kLAST = INT_MAX - kMAX*10;
   int v = 0;
   for ( ; *s != 0; s++) {
      int vadd = (*s)-'0';
      //printf("compare: %d kMAX: %d, v*10: %d, vadd %d, kLAST %d\n", v, kMAX, v*10, vadd, kLAST);
      if (v > kMAX)
         return -1;
      if (v == kMAX && vadd > kLAST) {
         return -1;
      }
      v = v*10 + vadd;
   }
   return v;
}

static int matoi_with_overflow(const char* s)
{
   const int kMIN = INT_MIN/10;
   const int kLAST = -(INT_MIN - kMIN*10);
   int v = 0;
   for ( ; *s != 0; s++) {
      int vadd = (*s)-'0';
      //printf("compare: %d kMIN: %d, v*10: %d, vadd %d, kLAST %d\n", v, kMIN, v*10, vadd, kLAST);
      if (v < kMIN)
         return 1;
      if (v == kMIN && vadd > kLAST) {
         return 1;
      }
      v = v*10 - vadd;
   }
   return v;
}

static void test_atoi_with_overflow_value(int sign, const char*s, int v)
{
   int vv;
   if (sign > 0)
      vv = patoi_with_overflow(s);
   else
      vv = matoi_with_overflow(s);

   //printf("atoi test: [%s] [%s] -> %d (0x%x) should be %d (0x%x)\n", (sign>0)?"+":"-", s, vv, vv, v, v);

   if (vv == v)
      return;

   printf("atoi test failed: [%s] [%s] -> %d (0x%x) != %d (0x%x)\n", (sign>0)?"+":"-", s, vv, vv, v, v);
   assert(!"mjson self test: my atoi() is broken, bye!");
   abort();
   // DOES NOT RETURN
}

static void test_atoi_with_overflow()
{
   int i = 0x80000000;
   // test positive values
   test_atoi_with_overflow_value(1, "0", 0);
   test_atoi_with_overflow_value(1, "1", 1);
   test_atoi_with_overflow_value(1, "12", 12);
   test_atoi_with_overflow_value(1, "1234", 1234);
   if (i < 0) { // check overflow of 32-bit integers
      test_atoi_with_overflow_value(1, "2147483646", 0x80000000-2);
      test_atoi_with_overflow_value(1, "2147483647", 0x80000000-1);
      test_atoi_with_overflow_value(1, "2147483648", -1);
      test_atoi_with_overflow_value(1, "2147483649", -1);
   }
   test_atoi_with_overflow_value(1, "999999999999999999999999999999999999999999999999999999", -1);

   // test negative
   test_atoi_with_overflow_value(-1, "0", 0);
   test_atoi_with_overflow_value(-1, "1", -1);
   test_atoi_with_overflow_value(-1, "12", -12);
   test_atoi_with_overflow_value(-1, "1234", -1234);
   if (i < 0) { // check overflow of 32-bit integers
      test_atoi_with_overflow_value(-1, "2147483646", 0x80000000+2);
      test_atoi_with_overflow_value(-1, "2147483647", 0x80000000+1);
      test_atoi_with_overflow_value(-1, "2147483648", 0x80000000+0);
      test_atoi_with_overflow_value(-1, "2147483649", 1);
      test_atoi_with_overflow_value(-1, "2147483650", 1);
   }
   test_atoi_with_overflow_value(-1, "999999999999999999999999999999999999999999999999999999", 1);
}

static MJsonNode* parse_number(const char* sin, const char* s, const char** sout)
{
   //printf("number-->%s\n", s);

   static int once = 1;
   if (once) {
      once = 0;
      test_atoi_with_overflow();
   }

   // per RFC 4627
   // A number contains an integer component that
   // may be prefixed with an optional minus sign, which may be followed by
   // a fraction part and/or an exponent part.
   //
   // number = [ minus ] int [ frac ] [ exp ]
   //      decimal-point = %x2E       ; .
   //      digit1-9 = %x31-39         ; 1-9
   //      e = %x65 / %x45            ; e E
   //      exp = e [ minus / plus ] 1*DIGIT
   //      frac = decimal-point 1*DIGIT
   //      int = zero / ( digit1-9 *DIGIT )
   //      minus = %x2D               ; -
   //      plus = %x2B                ; +
   //      zero = %x30                ; 0

   int sign = 1;
   std::string sint;
   std::string sfrac;
   int expsign = 1;
   std::string sexp;

   if (*s == '-') {
      sign = -1;
      s++;
   }

   if (*s == '0') {
      sint += *s;
      s++;
   } else {
      sint = parse_digits(s, sout);
      s = *sout;
   }

   if (*s == '.') {
      s++;
      sfrac = parse_digits(s, sout);
      s = *sout;
   }

   if (*s == 'e' || *s == 'E') {
      s++;

      if (*s == '-') {
         expsign = -1;
         s++;
      }

      if (*s == '+') {
         expsign = +1;
         s++;
      }

      sexp = parse_digits(s, sout);
      s = *sout;
   }

   //printf("number: sign %d, sint [%s], sfrac [%s], expsign %d, sexp [%s]\n", sign, sint.c_str(), sfrac.c_str(), expsign, sexp.c_str());

   // check for floatign point

   if (expsign < 0 || sfrac.length() > 0) {
      // definitely floating point number
      double v1 = atof(sint.c_str());
      double v2 = 0;
      double vm = 0.1;
      const char* p = sfrac.c_str();
      for ( ; *p != 0; p++, vm/=10.0) {
         v2 += (*p-'0')*vm;
      }

      int e = patoi_with_overflow(sexp.c_str());

      if (e < 0 || e > 400) {
         // overflow or exponent will not fit into IEEE754 double precision number
         // convert to 0 or +/- infinity
         printf("overflow!\n");
         if (expsign > 0) {
            *sout = s;
            double one = 1;
            double zero = 0;
            double inf = one/zero; // IEEE-754 1.0/0.0 is "+infinity", +infinity*(-1) => -infinity
            return MJsonNode::MakeNumber(sign*inf);
         } else {
            *sout = s;
            return MJsonNode::MakeNumber(sign*0.0);
         }
      }

      double ee = 1.0;
      if (e != 0)
         ee = pow(10, expsign*e);
      double v = sign*(v1+v2)*ee;
      //printf("v1: %f, v2: %f, e: %d, ee: %g, v: %g\n", v1, v2, e, ee, v);

      *sout = s;
      return MJsonNode::MakeNumber(v);
   } else {
      // no sfrac, expsign is positive, so this is an integer, unless it overflows

      int e = patoi_with_overflow(sexp.c_str()); // may overflow

      if (e < 0 || e > 400) {
         // overflow or exponent will not fit into IEEE754 double precision number
         // convert to +/- infinity
         //printf("overflow!\n");
         *sout = s;
         double one = 1;
         double zero = 0;
         double inf = one/zero; // IEEE-754 1.0/0.0 is "+infinity", +infinity*(-1) => -infinity
         return MJsonNode::MakeNumber(sign*inf);
      }

      // this is stupid but quicker than calling pow(). Unless they feed us stupid exponents that are not really integers anyway
      for (int ee=0; ee<e; ee++)
         sint += "0";

      int overflow = 0;
      int v = 0;

      if (sign > 0) {
         v = patoi_with_overflow(sint.c_str());
         if (v < 0)
            overflow = 1;
      } else {
         v = matoi_with_overflow(sint.c_str());
         if (v > 0)
            overflow = 1;
      }

      if (overflow) {
         // overflow, convert to double
         //printf("integer overflow!\n");
         double vv = atof(sint.c_str());
         *sout = s;
         return MJsonNode::MakeNumber(sign*vv);
      }

      *sout = s;
      return MJsonNode::MakeInt(v);
   }

   *sout = s;
   return MJsonNode::MakeError(NULL, "cannot parse number", sin, s);
}

static MJsonNode* parse_null(const char* sin, const char* s, const char** sout)
{
   if (s[0] == 'n' && s[1] == 'u' && s[2] == 'l' && s[3] == 'l') {
      *sout = s+4;
      return MJsonNode::MakeNull();
   }

   *sout = s;
   return MJsonNode::MakeError(NULL, "cannot parse \"null\"", sin, s);
}

static MJsonNode* parse_true(const char* sin, const char* s, const char** sout)
{
   if (s[0] == 't' && s[1] == 'r' && s[2] == 'u' && s[3] == 'e') {
      *sout = s+4;
      return MJsonNode::MakeBool(true);
   }

   *sout = s;
   return MJsonNode::MakeError(NULL, "cannot parse \"true\"", sin, s);
}

static MJsonNode* parse_false(const char* sin, const char* s, const char** sout)
{
   if (s[0] == 'f' && s[1] == 'a' && s[2] == 'l' && s[3] == 's' && s[4] == 'e') {
      *sout = s+5;
      return MJsonNode::MakeBool(false);
   }

   *sout = s;
   return MJsonNode::MakeError(NULL, "cannot parse \"false\"", sin, s);
}


static MJsonNode* parse_something(const char* sin, const char* s, const char** sout)
{
   s = skip_spaces(s);
   
   if (*s == '[') {
      return parse_array(sin, s+1, sout);
   } else if (*s == '{') {
      return parse_object(sin, s+1, sout);
   } else if (*s == '\"') {
      return parse_string(sin, s+1, sout);
   } else if (*s == '-') {
      return parse_number(sin, s, sout);
   } else if (*s >= '0' && *s <= '9') {
      return parse_number(sin, s, sout);
   } else if (*s == 'n') {
      return parse_null(sin, s, sout);
   } else if (*s == 't') {
      return parse_true(sin, s, sout);
   } else if (*s == 'f') {
      return parse_false(sin, s, sout);
   }

   *sout = s;
   return MJsonNode::MakeError(NULL, "unexpected char at top level", sin, s);
}

MJsonNode* MJsonNode::Parse(const char* jsonstring)
{
   const char*sout;
   return parse_something(jsonstring, jsonstring, &sout);
}

MJsonNode::~MJsonNode() // dtor
{
   for (unsigned i=0; i<subnodes.size(); i++)
      delete subnodes[i];
   subnodes.clear();

   // poison deleted nodes
   type = MJSON_NONE;
}

static char toHexChar(int c)
{
   assert(c>=0);
   assert(c<=15);
   if (c <= 9)
      return '0' + c;
   else
      return 'A' + c;
}

static std::string quote(const char* s)
{
   std::string v;
   while (*s) {
      switch (*s) {
      case '\"': v += "\\\""; s++; break;
      case '\\': v += "\\\\"; s++; break;
      //case '/': v += "\\/"; s++; break;
      case '\b': v += "\\b"; s++; break;
      case '\f': v += "\\f"; s++; break;
      case '\n': v += "\\n"; s++; break;
      case '\r': v += "\\r"; s++; break;
      case '\t': v += "\\t"; s++; break;
      default: {
         if (iscntrl(*s)) {
            v += "\\u";
            v += "0";
            v += "0";
            v += toHexChar(((*s)>>4) & 0xF);
            v += toHexChar(((*s)>>0) & 0xF);
            s++;
            break;
         } else {
            v += *s; s++;
            break;
         }
      }
      }
   }
   return v;
}
   
std::string MJsonNode::Stringify(int flags) const
{
   switch (type) {
   case MJSON_ARRAY: {
      std::string v;
      v += "[";
      for (unsigned i=0; i<subnodes.size(); i++) {
         if (i > 0)
            v += ",";
         v += subnodes[i]->Stringify(flags);
      }
      v += "]";
      return v;
   }
   case MJSON_OBJECT: {
      std::string v;
      v += "{";
      for (unsigned i=0; i<objectnames.size(); i++) {
         if (i > 0)
            v += ",";
         v += std::string("\"") + quote(objectnames[i].c_str()) + "\"";
         v += ":";
         v += subnodes[i]->Stringify(flags);
      }
      v += "}";
      return v;
   }
   case MJSON_STRING: {
      return std::string("\"") + quote(stringvalue.c_str()) + "\"";
   }
   case MJSON_INT: {
      char buf[256];
      sprintf(buf, "%d", intvalue);
      return buf;
   }
   case MJSON_NUMBER: {
      char buf[256];
      sprintf(buf, "%g", numbervalue);
      return buf;
   }
   case MJSON_BOOL:
      if (intvalue)
         return "true";
      else
         return "false";
   case MJSON_NULL:
      return "null";
   case MJSON_ERROR:
      return std::string("json parse error: ") + stringvalue;
   default:
      assert(!"should not come here");
      return ""; // NOT REACHED
   }
}

MJsonNode* MJsonNode::MakeError(MJsonNode* errornode, const char* errormessage, const char* sin, const char* serror)
{
   MJsonNode* n = new MJsonNode();
   n->type = MJSON_ERROR;
   if (errornode)
      n->subnodes.push_back(errornode);
   n->stringvalue = errormessage;
   if (sin && serror) {
      char msg[256];
      char sample[32];
      strncpy(sample, serror, 31);
      sample[31] = 0;
      int offset = serror-sin;
      int lineno = 1;
      int lineoff = 0;
      for (const char* s = sin; s != serror; s++) {
         if (*s == 0)
            break;
         if (*s == '\n') {
            lineno++;
            lineoff=0;
         } else {
            lineoff++;
         }
      }
      sprintf(msg, " at char \"%c\" file offset %d, line %d position %d, around text \"%s\"", *serror, offset, lineno, lineoff, sample);
      n->stringvalue += msg;
   }
   return n;
}

MJsonNode* MJsonNode::MakeArray()
{
   MJsonNode* n = new MJsonNode();
   n->type = MJSON_ARRAY;
   return n;
}

MJsonNode* MJsonNode::MakeObject()
{
   MJsonNode* n = new MJsonNode();
   n->type = MJSON_OBJECT;
   return n;
}

MJsonNode* MJsonNode::MakeString(const char* value)
{
   MJsonNode* n = new MJsonNode();
   n->type = MJSON_STRING;
   n->stringvalue = value;
   return n;
}

MJsonNode* MJsonNode::MakeInt(int value)
{
   MJsonNode* n = new MJsonNode();
   n->type = MJSON_INT;
   n->intvalue = value;
   n->numbervalue = value;
   return n;
}

MJsonNode* MJsonNode::MakeNumber(double value)
{
   MJsonNode* n = new MJsonNode();
   n->type = MJSON_NUMBER;
   n->numbervalue = value;
   return n;
}

MJsonNode* MJsonNode::MakeBool(bool value)
{
   MJsonNode* n = new MJsonNode();
   n->type = MJSON_BOOL;
   if (value)
      n->intvalue = 1;
   else
      n->intvalue = 0;
   return n;
}

MJsonNode* MJsonNode::MakeNull()
{
   MJsonNode* n = new MJsonNode();
   n->type = MJSON_NULL;
   return n;
}

void MJsonNode::AddToArray(MJsonNode* node)
{
   if (type == MJSON_ARRAY) {
      subnodes.push_back(node);
      return;
   }

   assert(!"not an array");
}

void MJsonNode::AddToObject(const char* name, MJsonNode* node) /// add node to an object
{
   if (type == MJSON_OBJECT) {
      objectnames.push_back(name);
      subnodes.push_back(node);
      //objectvalue[name] = node;
      return;
   }

   assert(!"not an object");
}

int MJsonNode::GetType() const /// get node type: MJSON_xxx
{
   return type;
}

const MJsonNodeVector* MJsonNode::GetArray() const
{
   if (type == MJSON_ARRAY || type == MJSON_NULL)
      return &subnodes;
   else
      return NULL;
}

const MJsonStringVector* MJsonNode::GetObjectNames() const
{
   if (type == MJSON_OBJECT || type == MJSON_NULL)
      return &objectnames;
   else
      return NULL;
}

const MJsonNodeVector* MJsonNode::GetObjectNodes() const
{
   if (type == MJSON_OBJECT || type == MJSON_NULL)
      return &subnodes;
   else
      return NULL;
}

const MJsonNode* MJsonNode::FindObjectNode(const char* name) const
{
   if (type != MJSON_OBJECT)
      return NULL;
   for (unsigned i=0; i<objectnames.size(); i++)
      if (strcmp(objectnames[i].c_str(), name) == 0)
         return subnodes[i];
   return NULL;
}

std::string MJsonNode::GetString() const
{
   if (type == MJSON_STRING)
      return stringvalue;
   else
      return "";
}

int MJsonNode::GetInt() const
{
   if (type == MJSON_INT)
      return intvalue;
   else
      return 0;
}

double MJsonNode::GetNumber() const
{
   if (type == MJSON_INT)
      return intvalue;
   else if (type == MJSON_NUMBER)
      return numbervalue;
   else if (type == MJSON_STRING) {
      if (stringvalue == "NaN") {
         double zero1 = 0;
         double zero2 = 0;
         return zero1/zero2; // IEEE-754 0.0/0.0 is a NaN
      } else if (stringvalue == "Infinity") {
         double zero = 0;
         double one = 1;
         return one/zero; // IEEE-754 1.0/0.0 is +infinity
      } else if (stringvalue == "-Infinity") {
         double zero = 0;
         double one = -1;
         return one/zero; // IEEE-754 -1.0/0.0 is -infinity
      } else if (stringvalue[0]=='0' && stringvalue[1]=='x') {
         return strtoul(stringvalue.c_str(), NULL, 16);
      }
      return 0;
   } else
      return 0;
}

bool MJsonNode::GetBool() const /// get boolean value, false if not a boolean or value is JSON "null"
{
   if (type == MJSON_BOOL)
      return (intvalue != 0);
   else
      return false;
}

std::string MJsonNode::GetError() const
{
   if (type == MJSON_ERROR)
      return stringvalue;
   else
      return "";
}

MJsonNode::MJsonNode() // private constructor
{
   // C++ does not know how to initialize elemental types, we have to do it by hand:
   type = MJSON_NONE;
   intvalue = 0;
   numbervalue = 0;
}

const char* MJsonNode::TypeToString(int type)
{
   switch (type) {
   default: return "UNKNOWN";
   case MJSON_ERROR: return "ERROR";
   case MJSON_NONE: return "NONE";
   case MJSON_ARRAY: return "ARRAY";
   case MJSON_OBJECT: return "OBJECT";
   case MJSON_STRING: return "STRING";
   case MJSON_INT: return "INT";
   case MJSON_NUMBER: return "NUMBER";
   case MJSON_BOOL: return "BOOL";
   case MJSON_NULL: return "NULL";
   }
}

static void pnest(int nest)
{
   for (int i=0; i<nest; i++)
      printf("  ");
}

void MJsonNode::Dump(int nest) const // debug
{
   printf("Node type %d (%s)", type, TypeToString(type));
   switch (type) {
   default: printf("\n"); break;
   case MJSON_STRING: printf(", value [%s]\n", stringvalue.c_str()); break;
   case MJSON_INT: printf(", value %d\n", intvalue); break;
   case MJSON_NUMBER: printf(", value %g\n", numbervalue); break;
   case MJSON_BOOL: printf(", value %d\n", intvalue); break;
   case MJSON_ARRAY:
      printf("\n");
      for (unsigned i=0; i<subnodes.size(); i++) {
         pnest(nest);
         printf("element %d: ", i);
         subnodes[i]->Dump(nest+1);
      }
      break;
   case MJSON_OBJECT:
      printf("\n");
      for (unsigned i=0; i<objectnames.size(); i++) {
         pnest(nest);
         printf("%s: ", objectnames[i].c_str());
         subnodes[i]->Dump(nest+1);
      }
      break;
   case MJSON_ERROR:
      printf(": %s\n", stringvalue.c_str());
      for (unsigned i=0; i<subnodes.size(); i++) {
         pnest(nest);
         printf("errorelement %d: ", i);
         subnodes[i]->Dump(nest+1);
      }
      break;
   }
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
