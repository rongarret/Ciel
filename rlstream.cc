/////////////////////////////////
///
///  A minimal C++ stream-like interface to readline.  This really ought
///  to be done with a basic_streambuf<char> (I think) but that doesn't
///  seem to work in g++ 2.96.  :-(
///
///  Update 8/2010: g++ is now up to version 4 so it might be worthwhile
///  trying basic_streambuf again.  On the other hand, what's here does
///  seem to work.
///

#include <unistd.h>  // For isatty()

#ifdef USE_GNU_READLINE

#include <stdio.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>

#else

// Provide a minimalist alternative in case the user doesn't have
// readline installed.

#include <iostream>
#include <string>

using namespace std;

char *readline(const char* prompt) {
  string s;
  if (isatty(0)) {   // 0=stdin
    cout << prompt;
    cout.flush();
  }
  getline(cin, s);
  if (cin.eof()) return 0;
  char *b = (char *)malloc(strlen(s.c_str()));  // Hm, need to heck for leaks
  strcpy(b, s.c_str());
  return b;
}

void add_history(char *) {}

#endif

//////////////////////////////////
//
// Actual rlstream code starts here
//

#include <stdlib.h>
#include <cctype>       // For isspace
#include "rlstream.h"

rlstream::rlstream(char * _prompt) {
  ptr = buffer = 0;
  prompt = _prompt;
}

int rlstream::eof() { return !ptr; }

rlstream& rlstream::operator>>(char& c) {
  do get(c); while (isspace(c) && !eof());
  return *this;
}

rlstream& rlstream::get(char& c) {
  c = peek();
  if (c) ptr++;
  return *this;
}

int rlstream::peek() {
 loop:
  if (buffer && ptr && *ptr) return *ptr;
  if (buffer) {
    free(buffer);
    buffer = 0;
    return '\n';
  }
  if (!isatty(0)) prompt="";
  ptr = buffer = readline(prompt);
  if (ptr && *ptr) add_history(ptr);
  if (ptr) goto loop;
  return 0;
}
