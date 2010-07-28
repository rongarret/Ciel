#include <unistd.h>  // For isatty()

#ifdef USE_GNU_READLINE

#include <stdio.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>

#else

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
  char *b = (char *)malloc(strlen(s.c_str()));
  strcpy(b, s.c_str());
  return b;
}

void add_history(char *) {}

#endif

#include <stdlib.h>
#include "rlstream.h"

/////////////////////////////////
///
///  A minimal C++ stream-like interface to readline.  This really ought
///  to be done with a basic_streambuf<char> (I think) but that doesn't
///  seem to work in g++ 2.96.  :-(
///

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
