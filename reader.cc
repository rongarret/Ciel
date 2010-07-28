#include "cl.h"
#include "rlstream.h"

//////////////////////////
//
//  Reader
//

rlstream clin;

string readToken(string terminators = "()[]{}',") {
  char c;
  string s = "";
  clin >> c;  // Skip whitespace
  if (clin.eof()) {
    cout << "\nSee ya!\n";
    exit(0);
  }

  // Terminators are tokens
  if (terminators.find_first_of(c) != string::npos) return s+c;

  // Read strings - there's a horrible hack here.  Strings get returned
  // with the leading double quote but not the trailing double quote.
  // the code that turns the string into a clString then strips off
  // the leading quote.  Surrounding quotes then get added again by
  // the print method for clString.  There has to be a better way.
  if (c=='"') {
    clin.prompt="\"... ";
    do {
      s += c;
      clin.get(c);
      if (clin.eof()) exit(0);
    } while (c != '"');
    return s;
  }

  // Read symbols and numbers (which one we're reading gets sorted out later)
  // BUG: 123"foo"456 reads as a symbol.  (Or is this a feature?)
  while(1) {
    s += c;
    c = clin.peek();
    if (clin.eof()) exit(0);
    if (isspace(c) || terminators.find_first_of(c) != string::npos) return s;
    clin.get(c);
  }
}



clObject& readObject(string parenStack = "... ") {
  clin.prompt = (parenStack[0] == '.' ? "Ciel: " : parenStack.c_str());
  string s;
 start:
  s = readToken();

  if (s==")") {
    if (parenStack[0]=='(') return nil;
    cout << "Ignored extra right paren\n";
    goto start;
  }

  // Handle dotted pair notation
  if (s==".") {
    if (parenStack[0] != '(') {
      cout << "Dot context error\n";
      goto start;
    }
    clObject &o = readObject();
    s = readToken();
    if (s != ")") {
      cout << "Syntax error reading dotted pair.  Expected close paren, got "
	   << s << '\n';
      goto start;
    }
    return o;
  }

  clObject *o;
  if (s=="(") {
    o = &readObject('(' + parenStack);
  } else if (s=="[") {
    string p = '[' + parenStack;
    clVector* v = new clVector();
    clObject* o1 = &readObject(p);
    while (o1 != &intern("]")) {
      vpe(*v, *o1);
      o1 = &readObject(p);
    }
    o = v;
  } else if (s=="{") {
    string p = '{' + parenStack;
    clObject* o1 = &readObject(p);
    while (o1 != &intern("}")) {
      //      vpe(*v, *o1);
      o1 = &readObject(p);
    }
    o = new dictionary;
  } else if (s=="+" || s=="-" || s == "i") {
    // CLN incorrectly parses "+", "-" and "i" as numbers (as 0 actually)
    // So we have to handle these as special cases before we invoke the
    // CLN number parser.
    o = &intern(s);
  } else if (s=="'") {
    o = &cons(intern("quote"), cons(readObject(), nil));
  } else if (s[0]=='"') {
    o = new clString(s.substr(1));
  } else try {
    o = new clNumber(s.c_str());
  } catch(...) {
    o = &intern(s);
  }

  return parenStack[0] == '(' ? cons(*o, readObject(parenStack)) : *o;
}
