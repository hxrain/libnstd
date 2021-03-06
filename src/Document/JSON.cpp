
#include <nstd/Buffer.h>
#include <nstd/Unicode.h>
#include <nstd/Error.h>
#include <nstd/Document/JSON.h>

class JSON::Private
{
public:
  class Position
  {
  public:
    int line;
    const tchar* pos;
  };

  class Token
  {
  public:
    tchar token;
    Variant value;
    Position pos;
  };

public:
  Token token;
  const tchar* start;
  Position pos;

  int errorLine;
  int errorColumn;
  String errorString;

public:
  bool parse(const tchar* data, Variant& result);

  bool parseObject(Variant& result);
  bool parseValue(Variant& result);
  bool parseArray(Variant& result);

  bool readToken();

  void skipSpace();

  void syntaxError(const Position& pos, const String& error);

public:
  static void appendEscapedString(const String& str, String& result);
  static void appendVariant(const Variant& data, const String& indentation, String& result);
};

class JSON::Parser::Private : public JSON::Private
{
};

bool JSON::Private::readToken()
{
  skipSpace();
  token.token = *pos.pos;
  token.pos = pos;
  switch(token.token)
  {
  case '\0':
    return true;
  case '{':
  case '}':
  case '[':
  case ']':
  case ',':
  case ':':
    ++pos.pos;
    return true;
  case '"':
    {
      ++pos.pos;
      String value;
      for(;;)
        switch(*pos.pos)
        {
        case '\0':
          return syntaxError(pos, _T("Unexpected end of file")), false;
        case '\r':
          if(*(++pos.pos) == '\n')
            ++pos.pos;
          ++pos.line;
          continue;
        case '\n':
          ++pos.line;
          ++pos.pos;
          continue;
        case '\\':
          {
            ++pos.pos;
            switch(*pos.pos)
            {
            case '"':
            case '\\':
            case '/':
              value.append(*pos.pos);
              ++pos.pos;
              break;
            case 'b':
              value.append('\b');
              ++pos.pos;
              break;
            case 'f':
              value.append('\f');
              ++pos.pos;
              break;
            case 'n':
              value.append('\n');
              ++pos.pos;
              break;
            case 'r':
              value.append('\r');
              ++pos.pos;
              break;
            case 't':
              value.append('\t');
              ++pos.pos;
              break;
            case 'u':
              {
                ++pos.pos;
                String k(4);
                for(int i = 0; i < 4; ++i)
                  if(String::isHexDigit(*pos.pos))
                  {
                    k.append(*pos.pos);
                    ++pos.pos;
                  }
                  else
                    return syntaxError(pos, _T("Expected hexadecimal digit")), false;
                uint w1;
                if(k.scanf(_T("%x"), &w1) == 1)
                  return pos.pos -= 4, syntaxError(pos, _T("Expected hexadecimal number")), false;
                if((w1 & 0xF800ULL) == 0xD800ULL && (w1 & 0xFC00ULL) == 0xD800ULL)
                { // this must be an UTF-8 surrogate pair
                  if(*pos.pos != '\\' || pos.pos[1] != 'u')
                    return syntaxError(pos, _T("Expected UTF-8 surrogate pair")), false;
                  pos.pos += 2;
                  k.clear();
                  for(int i = 0; i < 4; ++i)
                    if(String::isHexDigit(*pos.pos))
                    {
                      k.append(*pos.pos);
                      ++pos.pos;
                    }
                    else
                      return syntaxError(pos, _T("Expected hexadecimal digit")), false;
                  uint w2;
                  if(k.scanf(_T("%x"), &w2) == 1)
                    return pos.pos -= 4, syntaxError(pos, _T("Expected hexadecimal number")), false;
                  if((w2 & 0xFC00UL) != 0xDC00UL)
                    return pos.pos -= 6,syntaxError(pos, _T("Expected UTF-8 surrogate pair")), false;
                  Unicode::append(((w2 & 0x3FFULL) | ((uint32)(w1 & 0x3FFULL) << 10)) + 0x10000UL, value);
                }
                else
                  Unicode::append(w1, value);
                break;
              }
              break;
            default:
              value.append('\\');
              value.append(*pos.pos);
              ++pos.pos;
              break;
            }
          }
          break;
        case '"':
          ++pos.pos;
          token.value = value;
          return true;
        default:
          value.append(*pos.pos);
          ++pos.pos;
          break;
        }
    }
    return false; // unreachable
  case 't':
    if(String::compare(pos.pos, _T("true"), 4) == 0)
    {
      pos.pos += 4;
      token.value = true;
      return true;
    }
    return syntaxError(pos, _T("Expected character")), false;
  case 'f':
    if(String::compare(pos.pos, _T("false"), 5) == 0)
    {
      pos.pos += 5;
      token.value = false;
      return true;
    }
    return syntaxError(pos, _T("Expected character")), false;
  case 'n':
    if(String::compare(pos.pos, _T("null"), 4) == 0)
    {
      pos.pos += 4;
      token.value.clear(); // creates a null variant
      return true;
    }
    return syntaxError(pos, _T("Expected character")), false;
  default:
    token.token = '#';
    if(*pos.pos == '-' || String::isDigit(*pos.pos))
    { // todo: sophisticated number format checking
      String n;
      bool isDouble = false;
      for(;;)
        switch(*pos.pos)
        {
        case 'E':
        case 'e':
        case '-':
        case '+':
          n.append(*pos.pos);
          ++pos.pos;
          break;
        case '.':
          isDouble = true;
          n.append(*pos.pos);
          ++pos.pos;
          break;
        default:
          if(String::isDigit(*pos.pos))
          {
            n.append(*pos.pos);
            ++pos.pos;
            break;
          }
          goto scanNumber;
        }
    scanNumber:
      if(isDouble)
      {
        token.value = n.toDouble();
        return true;
      }
      else
      {
        int64 result = n.toInt64();
        int resultInt = (int)result;
        if((int64)resultInt == result)
          token.value = resultInt;
        else
          token.value = result;
        return true;
      }
    }
    return syntaxError(pos, _T("Expected character")), false;
  }
}


void JSON::Private::skipSpace()
{
  for(tchar c;;)
    switch((c = *pos.pos))
    {
    case '\r':
      if(*(++pos.pos) == '\n')
        ++pos.pos;
      ++pos.line;
      continue;
    case '\n':
      ++pos.line;
      ++pos.pos;
      continue;
    default:
      if(String::isSpace(c))
        ++pos.pos;
      else
        return;
    }
}

void JSON::Private::syntaxError(const Position& pos, const String& error)
{
  int column = 1;
  for(const tchar* p = pos.pos; p > start;)
  {
    --p;
    if(*p == '\n' || *p == '\r')
      break;
    ++column;
  }
  errorLine = pos.line;
  errorColumn = column;
  errorString = error;
}

bool JSON::Private::parseObject(Variant& result)
{
  if(token.token != '{')
    return syntaxError(pos, _T("Expected '{'")), false;
  if(!readToken())
    return false;
  HashMap<String, Variant>& object = result.toMap();
  String key;
  while(token.token != '}')
  {
    if(token.token != '"')
      return syntaxError(pos, _T("Expected '\"'")), false;
    key = token.value.toString();
    if(!readToken())
      return false;
    if(token.token != ':')
      return syntaxError(pos, _T("Expected ':'")), false;
    if(!readToken())
      return false;
    if(!parseValue(object.append(key, Variant())))
      return false;
    if(token.token == '}')
      break;
    if(token.token != ',')
      return syntaxError(pos, _T("Expected ','")), false;
    if(!readToken())
      return false;
  } 
  if(!readToken()) // skip }
    return false;
  return true;
}

bool JSON::Private::parseArray(Variant& result)
{
  if(token.token != '[')
    return syntaxError(pos, _T("Expected '['")), false;
  if(!readToken())
    return false;
  List<Variant>& list = result.toList();
  while(token.token != ']')
  {
    if(!parseValue(list.append(Variant())))
      return false;
    if(token.token == ']')
      break;
    if(token.token != ',')
      return syntaxError(pos, _T("Expected ','")), false;
    if(!readToken())
      return false;
  }
  if(!readToken()) // skip ]
    return false;
  return true;
}

bool JSON::Private::parseValue(Variant& result)
{
  switch(token.token)
  {
  case '"':
  case '#':
  case 't':
  case 'f':
  case 'n':
    {
      result = token.value;
      if(!readToken())
        return false;
      return true;
    }
  case '[':
    return parseArray(result);
  case '{':
    return parseObject(result);
  }
  return syntaxError(pos, _T("Unexpected character")), false;
}


void JSON::Private::appendEscapedString(const String& str, String& result)
{
  usize strLen = str.length();
  result.reserve(result.length() + 2 + strLen * 2);
  result += '"';
  for(const tchar* start = str, * p = start;;)
  {
    const tchar* e = String::findOneOf(p, _T("\"\\"));
    if(!e)
    {
      result.append(p, strLen - (p - start));
      break;
    }
    if(e > p)
      result.append(p, e - p);
    switch(*e)
    {
    case '"':
      result += _T("\\\"");
      break;
    case '\\':
      result += _T("\\\\");
      break;
    }
    p = e + 1;
  }
  result += '"';
}

void JSON::Private::appendVariant(const Variant& data, const String& indentation, String& result)
{
  switch(data.getType())
  {
  case Variant::nullType:
    result += _T("null");
    return;
  case Variant::boolType:
  case Variant::doubleType:
  case Variant::intType:
  case Variant::uintType:
  case Variant::int64Type:
  case Variant::uint64Type:
    result += data.toString();
    return;
  case Variant::stringType:
    appendEscapedString(data.toString(), result);
    return;
  case Variant::mapType:
    {
      const HashMap<String, Variant>& map = data.toMap();
      if(map.isEmpty())
        result += _T("{}");
      else
      {
        result += _T("{\n");
        String newIndentation = indentation + _T("\t");
        for(HashMap<String, Variant>::Iterator i = map.begin(), end = map.end();;)
        {
          result += newIndentation;
          appendEscapedString(i.key(), result);
          result += _T(": ");
          appendVariant(*i, newIndentation, result);
          if(++i == end)
            break;
          result += _T(",\n");
        }
        result += '\n';
        result += indentation;
        result += '}';
      }
      return;
    }
  case Variant::listType:
    {
      const List<Variant>& list = data.toList();
      if(list.isEmpty())
        result += _T("[]");
      else
      {
        result += _T("[\n");;
        String newIndentation = indentation + _T("\t");
        for(List<Variant>::Iterator i = list.begin(), end = list.end();;)
        {
          result += newIndentation;
          appendVariant(*i, newIndentation, result);
          if(++i == end)
            break;
          result += _T(",\n");
        }
        result += '\n';
        result += indentation;
        result += ']';
      }
      return;
    }
  case Variant::arrayType:
    {
      const Array<Variant>& array = data.toArray();
      if(array.isEmpty())
        result += _T("[]");
      else
      {
        result += _T("[\n");;
        String newIndentation = indentation + _T("\t");
        for(Array<Variant>::Iterator i = array.begin(), end = array.end();;)
        {
          result += newIndentation;
          appendVariant(*i, newIndentation, result);
          if(++i == end)
            break;
          result += _T(",\n");
        }
        result += '\n';
        result += indentation;
        result += ']';
      }
      return;
    }
  }
}

bool JSON::Private::parse(const tchar* data, Variant& result)
{
  start = data;
  pos.line = 1;
  pos.pos = start;

  if(!readToken())
    return false;
  if(!parseValue(result))
    return false;
  return true;
}

bool JSON::parse(const tchar* data, Variant& result)
{
  Private parser;
  if(parser.parse(data, result))
    return true;
  Error::setErrorString(String::fromPrintf(_T("Syntax error at line %d, column %d: %s"), parser.errorLine, parser.errorColumn, (const tchar*)parser.errorString));
  return false;
}

bool JSON::parse(const String& data, Variant& result)
{
  return parse((const tchar*)data, result);
}

String JSON::toString(const Variant& data)
{
  String result;
  Private::appendVariant(data, String(), result);
  result += '\n';
  return result;
}

JSON::Parser::Parser() : p(new Private) {}
JSON::Parser::~Parser() {delete p;}
int JSON::Parser::getErrorLine() const {return p->errorLine;}
int JSON::Parser::getErrorColumn() const {return p->errorColumn;}
String JSON::Parser::getErrorString() const {return p->errorString;}
bool JSON::Parser::parse(const tchar* data, Variant& result) {return p->parse(data, result);}
bool JSON::Parser::parse(const String& data, Variant& result) {return p->parse(data, result);}
