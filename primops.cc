
#include "cl.h"

gc_symbol_hash_map symbolTable;
dEnv TLE;  // The top-level environment
clSymbol& clTrue = intern("T");

primop::primop(const char* n, clObject& (*a)(argsT&), int i) :
  name(n), applier(a), argcnt(i) {
  TLE.set(intern(name), *this);
}

void primop::print(ostream &os) { os << "<primop " << name << '>'; }

clObject& primop::apply(argsT& args) {
  if (argcnt && argcnt != length(args))
    error("Wrong number of arguments", *this);
  return (*applier)(args);
}

#define PRIMOP(name, nargs) \
  clObject& primop_ ## name ## _apply(argsT& args); \
  primop _ ## name(#name, &primop_ ## name ## _apply, nargs); \
  clObject& primop_ ## name ## _apply(argsT& args)


//////////////////////////////////////////////////////////////////////

PRIMOP(cons, 2) { return cons(args[0], args[1]); }
PRIMOP(car, 1) { return car(args[0]); }
PRIMOP(cdr, 1) { return cdr(args[0]); }

PRIMOP(plus, 0) {
  cl_N sum(0);
  gcVector::iterator start = args.theVector.begin();
  gcVector::iterator end = args.theVector.end();
  for (gcVector::iterator i = start; i!=end; i++)
    sum = sum + theNumber(**i);
  return *new clNumber(sum);
}

PRIMOP(times, 0) {
  cl_N product(1);
  gcVector::iterator start = args.theVector.begin();
  gcVector::iterator end = args.theVector.end();
  for (gcVector::iterator i = start; i!=end; i++)
    product = product * theNumber(**i);
  return *new clNumber(product);
}

PRIMOP(minus, 2) {
  return *new clNumber(theNumber(args[0]) - theNumber(args[1]));
}

PRIMOP(div, 2) {
  return *new clNumber(theNumber(args[0]) / theNumber(args[1]));
}

PRIMOP(lte, 2) {
  if (theReal(args[0]) <= theReal(args[1])) return clTrue;
  else return nil;
}

PRIMOP(gte, 2) {
  if (theReal(args[0]) >= theReal(args[1])) return clTrue;
  else return nil;
}

PRIMOP(lt, 2) {
  if (theReal(args[0]) < theReal(args[1])) return clTrue;
  else return nil;
}

PRIMOP(gt, 2) {
  if (theReal(args[0]) > theReal(args[1])) return clTrue;
  else return nil;
}

PRIMOP(eql, 2) {
  try {
    if (theNumber(args[0]) == theNumber(args[1]))
      return clTrue;
  } catch (bad_cast) {
    if (&args[0] == &args[1]) return clTrue;
  }
  return nil;
}

PRIMOP(sqrt, 1) { return *new clNumber(sqrt(theNumber(args[0]))); }
PRIMOP(log, 1) { return *new clNumber(log(theNumber(args[0]))); }

PRIMOP(log2, 2) {
  return *new clNumber(log(theNumber(args[0]), theNumber(args[1])));
}

PRIMOP(expt, 2) {
  return *new clNumber(expt(theNumber(args[0]), theNumber(args[1])));
}

PRIMOP(throw, 1) { throw(&args[0]); }

PRIMOP(print, 1) {
  cout << args[0] << '\n';
  return args[0];
}

PRIMOP(typeof, 1) { return *new clString(args[0].typeOf()); }

PRIMOP(system, 1) {
  return *new clNumber(system(the<clString>(args[0]).theThing.c_str()));
}

PRIMOP(vpe, 2) {
  vpe(the<clVector>(args[0]), args[1]);
  return args[0];
}

PRIMOP(vec, 0) {
  clVector& r = *new clVector();
  for (int i=0; i<length(args); i++) vpe(r, args[i]);
  return r;
}

PRIMOP(ref, 2) {
  try {
    return the<clVector>(args[0])[args[1]];
  } catch (bad_cast) {
    return the<clMap>(args[0]).ref(args[1]);
  }
}

PRIMOP(setref, 3) {
  try {
    vset(the<clVector>(args[0]), args[1], args[2]);
  } catch (bad_cast) {
    the<clMap>(args[0]).setref(args[1], args[2]);
  }
  return args[2];
}

PRIMOP(del, 2) {
  the<clMap>(args[0]).del(args[1]);
  return nil;
}

PRIMOP(make_dictionary, 0) {
  cout << args;
  return *new dictionary;
}

PRIMOP(len, 1) {
  try {
    clVector& v = dynamic_cast<clVector&>(args[0]);
    return *new clNumber(length(v));
  } catch (bad_cast) {
    consCell& c = dynamic_cast<consCell&>(args[0]);
    return *new clNumber(length(c));
  }
}


#include <dlfcn.h>
PRIMOP(dynload, 1) {
  const char* filename = the<clString>(args[0]).theThing.c_str();
  void* handle = dlopen(filename, RTLD_NOW);
  if (handle == NULL) {
    cout << dlerror() << '\n';
    return nil;
  }
  return args[0];
}

PRIMOP(gc, 0) {
  GC_gcollect();
  return *new clNumber(0);
}
