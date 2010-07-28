
#ifndef __RLSTREAM__
#define __RLSTREAM__

// Stub to use when libreadline is not available

class rlstream {
 public:
  char * buffer;
  char * ptr;
  const char * prompt;

  rlstream(char * = (char *)"");
  int eof();
  int peek();
  rlstream& get(char&);
  rlstream& operator>>(char&);
};

#endif
