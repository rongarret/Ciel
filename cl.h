#include <iostream>
#include <string>
#include <ext/hash_map>
#include <map>
#include <vector>
#include <typeinfo>  // For bad_cast exception type
#include <cln/cln.h>
#include <gc/gc_cpp.h>
#include <gc/gc_allocator.h>
// #include <gc/new_gc_alloc.h>
#include "stopwatch.h"

using namespace cln;
using namespace std;
using namespace __gnu_cxx;

string type_of(cl_N n);

////////////////////////
//
// Objects
//

class Env;       // Forward declaration needed so we can declare eval

class clObject : public gc {
public:
  virtual void print(ostream &s)=0;
  virtual string typeOf()=0;
  virtual clObject& eval(Env& e) { return *this; }
  virtual ~clObject() {}  // Do we need this?  Yes.  Why?  I don't know.

  inline operator int();
};

class clNumber : public clObject {
public:
  cl_N n;

  template<class T> inline clNumber(T s) { n = cl_N(s); }
  //  clNumber(int _n) { n = _n; }
  inline void print(ostream &s) { s << n; }
  inline string typeOf() { return type_of(n); }
};

ostream& operator<<(ostream &s, clObject &o);

//////////////////////
///
///  Type conversions
///
template<class T>
inline T* the(clObject *x) { return dynamic_cast<T*>(x); }
template<class T>
inline T& the(clObject &x) { return dynamic_cast<T&>(x); }

inline cl_N theNumber(clObject& o) { return the<clNumber>(o).n; }
inline cl_R theReal(clObject& o) { return As(cl_R)(the<clNumber>(o).n); }
inline cl_I theInteger(clObject& o) { return As(cl_I)(the<clNumber>(o).n); }

inline clObject::operator int() { return cl_I_to_int(theInteger(*this)); }

/////////////////////
///
///  Sequences
///
class clSequence : public clObject {
  virtual clObject& ref(int i)=0;
  virtual void refSet(int i, clObject* o)=0;
  virtual clSequence& slice(int i, int j)=0;
};


///////////////////////////////
//
//  Wrappers for native types (string and characters)
//
template<class T>
class clWrapper : public clObject {
  const static string name;
 public:
  T& theThing;
  clWrapper<T>() : theThing(*new T()) {}
  clWrapper<T>(T& x) : theThing(*new T(x)) {}
  clWrapper<T>(const T& x) : theThing(*new T(x)) {}
  string typeOf() { return name; }
  void print(ostream &os);
  virtual clObject& eval(Env& e) { return *this; }
};

//  Strings
typedef clWrapper<string> clString;

// Characters
typedef clWrapper<char> clChar;

/////////////////////////////
///
///  Dictionaries
///

// Some STL wizardry to make hashmaps work with strings and pointers
namespace __gnu_cxx {
  template<> struct hash<string> {
    int operator()(const string& x) const {
      return __stl_hash_string(x.c_str());
    }
  };

  template<> struct hash<void*> {
    inline int operator()(void* const &x) const { return (long)x; }
  };
}

typedef gc_allocator<clObject*> gc_alloc;

typedef
  hash_map<clObject*, clObject*, hash<void*>, equal_to<void*>,
	   gc_allocator<pair<clObject*, clObject*> > >
  gc_hash_map;

namespace std {
  template<> struct less<cl_R> {
    int operator()(const cl_R& x, const cl_R& y) const {
      return x<y;
    }
  };
}

typedef
  map<clObject*, clObject*, less<void*> >
  gc_map;

class clMap : public clObject {
 public:
  string typeOf() { return "<dictionary>"; }
  virtual clObject& ref(clObject&) = 0;
  virtual void setref(clObject&, clObject&) = 0;
  virtual clObject& size() = 0;
  virtual void del(clObject&) = 0;
};

template <class T>
class clMapImpl : public clMap {
 public:
  T theMap;
  virtual void print(ostream&);
  virtual clObject& ref(clObject&);
  virtual void setref(clObject&, clObject&);
  virtual clObject& size();
  virtual void del(clObject&);
};

template<class T>
void clMapImpl<T>::print(ostream &s) {
  // Needs a circularity check
  s << '{';
  //  T& m = theMap;
  typename T::iterator i = theMap.begin();
  if (i != theMap.end()) {
    s << *i->first << " : " << *i->second;
    for (i++; i != theMap.end(); i++) {
      s << ", " << *i->first << " : ";
      if (i->second) s << *i->second;
      else s << "<unbound>";
    }
  }
  s << '}';
}

typedef clMapImpl<gc_hash_map> dictionary;


///////////////////////////
//
// Vectors
//
typedef vector<clObject*, gc_alloc> gcVector;

extern out_of_range range_exception;

inline clObject& at(gcVector& v, int i) {
  if ((i<0) || (i>=(int)v.size())) throw(range_exception);
  return *(v[i]);
}

class clVector : public clObject {
 public:
  gcVector theVector;

  clVector() {}
  clVector(int n) : theVector(*new gcVector(n)) {}
  clVector(gcVector v) : theVector(v) {}

  string typeOf() { return "Vector"; }

  void print(ostream &s);
  clObject& eval(Env&);
  inline clObject& operator[](int);
  inline clObject& operator[](clObject&);
};

inline clObject& clVector::operator[](int n) {
  return at(this->theVector, n);
}

inline clObject& clVector::operator[](clObject& n) {
  return at(this->theVector, cl_I_to_int(theInteger(n)));
}

inline void vpe(clVector& v, clObject& o) {
  v.theVector.push_back(&o);
}

inline void vset(clVector& v, int i, clObject& o) {
  v.theVector[i] = &o;
}

inline int size(clVector& v) {
  return v.theVector.size();
}

// Function arguments are passed as clVectors
typedef clVector argsT;

//////////////////////////////
//
// Cons cells
//
class consCell : public clObject {
public:
  clObject *theCar;
  clObject *theCdr;

  void inline setCar(clObject& o) { theCar = &o; }
  void inline setCdr(clObject& o) { theCdr = &o; }

  string typeOf() { return "cons"; }
  void print(ostream &s);
  clObject& eval(Env&);
  clObject& operator[](int);
};

consCell& cons(clObject &car, clObject &cdr);

inline clObject& car(consCell& c) { return *(c.theCar); }
inline clObject& cdr(consCell& c) { return *(c.theCdr); }
inline clObject& car(consCell* c) { return *(c->theCar); }
inline clObject& cdr(consCell* c) { return *(c->theCdr); }

inline consCell* theCons(clObject *x) { return the<consCell>(x); }
inline consCell& theCons(clObject &x) { return the<consCell>(x); }

inline clObject& car(clObject& c) { return *(theCons(c).theCar); }
inline clObject& cdr(clObject& c) { return *(theCons(c).theCdr); }
inline clObject& car(clObject* c) { return *(theCons(c)->theCar); }
inline clObject& cdr(clObject* c) { return *(theCons(c)->theCdr); }

// NIL
class clNull : public consCell {
public:
  void print(ostream &s) { s << "Nil"; }
  string typeOf() { return "null"; }

  clNull() { theCar = this; theCdr = this; }
  clObject& eval(Env& e) { return *this; }
};

extern clNull nil;
extern clNull& NIL;

inline int null(clObject &o) { return &o==&nil; }
inline int null(clObject *o) { return o==&nil; }

///////////////////////////////////
//
// Symbols
//

class clSymbol : public clObject {
public:
  string name;

  clSymbol(string s) : name(s) {}

  void print(ostream &os) { os << name; }
  inline string typeOf() { return "Symbol"; }
  inline int operator==(clSymbol& s) { return this==&s; }
  clObject& eval(Env&);
  ~clSymbol() { cerr << "*** Destroying symbol " << name << '\n'; }

private:
  clSymbol& operator=(const clSymbol&);
  clSymbol(const clSymbol&);
};

clSymbol& intern(string s);
extern clSymbol& QUOTE;

typedef
  hash_map<string, clSymbol*, hash<string>, equal_to<string>,
	   gc_allocator<pair<string, clSymbol*> > >
  gc_symbol_hash_map;

extern gc_symbol_hash_map symbolTable;


/////////////////
///
///  Dynamic environments
///
class dEnv : public clObject {
 public:
  gc_map m;
  dEnv* parent;

  dEnv() : parent(NULL) {}

  void print (ostream& s);
  clObject& lkup(clSymbol& s);

  inline string typeOf() { return "environment"; }
  inline void set(clSymbol& s, clObject& o) { m[&s]=&o; }
};

extern dEnv TLE;  // Top level environment


////////////////
///
///  Primops
///
class primop : public clObject {
public:
  string name;
  clObject& (*applier)(argsT&);
  int argcnt;

  primop(const char* n, clObject& (*a)(argsT&), int i=0);

  void print(ostream &os);
  inline string typeOf() { return "Primop"; }

  clObject& apply(argsT& args);
};


/// Misc. hoohas
inline int length(clVector& v) { return v.theVector.size(); }
int length(consCell& c);
void error(string msg, clObject& arg);
typedef unsigned int Uint;

// Hack to force a call to GC_init before other static initializers
class GC_initter {
 public:
  GC_initter() { printf("Initializing GC\n"); GC_init(); }
};
