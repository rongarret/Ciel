#include "cl.h"

extern "C" clObject& vec(argsT& args) {
  //  cout << "foo\n";
  return args;
}

extern "C" clObject& baz(argsT& args) {
  for (int i=0; i<theInteger(args[0]); i++) {
    clVector& v = *new clVector();
    for (int j=0; j<theInteger(args[1]); j++) {
      vpe(v, nil);
    }
  }
  return nil;
}

extern "C" clObject& bar(argsT& args) {
  for (int i=0; i<theInteger(args[0]); i++) {
    consCell& v = cons(nil, nil);
    for (int j=0; j<theInteger(args[1]); j++) {
      v = cons(nil, v);
    }
  }
  return nil;
}

clNumber ONE("1");
clNumber TWO("2");

clObject& fib1(clObject& n) {
  if (theReal(n) <= theReal(ONE))
    return ONE;
  else
    return *new clNumber
      (theNumber(fib1(*new clNumber(theNumber(n)-theNumber(ONE)))) +
       theNumber(fib1(*new clNumber(theNumber(n)-theNumber(TWO)))));
}

extern "C" clObject& fibo(argsT& args) {
  return fib1(args[0]);
}

class initter {
public:
  initter() {
    new primop("vec", &vec);
    new primop("baz", &baz, 2);
    new primop("bar", &bar, 2);
    new primop("fibo", &fibo, 1);
  }
  ~initter() {
    cout << "Closing fftest\n";
  }
} _;

cl_I fib2(cl_R n) {
  if (n <= 1) return 1;
  else return fib2(n-1) + fib2(n-2);
}

int fib3(int n) {
  if (n <= 1) return 1;
  else return fib3(n-1) + fib3(n-2);
}
