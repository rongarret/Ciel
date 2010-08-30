#include "cl.h"
#include "reader.h"

ostream& operator<<(ostream &s, clObject &o) {
  o.print(s);
  return s;
}

/////////////////////////
///
///  Constants
///

clNull nil = clNull();
clNull& NIL = nil;
out_of_range range_exception("Out of range");


///////////////////////////
///
///  Strings
///

template<> const string clString::name = "String";

template<>
void clString::print(ostream &os) { os << '"' << theThing << '"'; }

///////////////////////
///
///  Characters
///

template<> const string clChar::name = "Character";

template<>
void clChar::print(ostream &os) { os << "'" << theThing << "'"; }

///////////////////////
///
///  Dictionaries
///

template<>
clObject& dictionary::ref(clObject& i) {
  gc_hash_map::iterator r = this->theMap.find(&i);
  if (r == this->theMap.end()) return nil;
  return *r->second;
}

template<>
void dictionary::del(clObject& i) {
  gc_hash_map::iterator r = this->theMap.find(&i);
  if (r == this->theMap.end()) return;
  this->theMap.erase(r);
}

template<>
void dictionary::setref(clObject &i, clObject &v) {
  this->theMap[&i] = &v;
}

template<>
clObject& dictionary::size() {
  return *new clNumber(this->theMap.size());
}


/////////////////////////////////////
//
//  Numbers
//

string type_of(cl_N n) {
  if (instanceof(n, cl_I_ring)) return "integer";
  else if (instanceof(n, cl_RA_ring)) return "rational";
  else if (instanceof(n, cl_R_ring)) return "float";
  else if (instanceof(n, cl_C_ring)) return "complex";
  else return "unknown";
}

//////////////////////////////
///
/// Vectors
///

int position(gcVector& v, clObject& o) {
  for (unsigned int i=0; i<v.size(); i++) {
    if (v[i]==&o) return i;
  }
  return -1;
}

void clVector::print(ostream &s) {
  s << '[';
  gcVector::iterator i = this->theVector.begin();
  gcVector::iterator end = this->theVector.end();
  if (i!=end) {
    s << **i;
    for (i++; i!=end; i++) s << ' ' << **i;
  }
  s << ']';
}


//////////////////////////
///
///  Cons Cells
///

consCell& cons(clObject &car, clObject &cdr) {
  consCell *c = new consCell();
  c->theCar = &car;
  c->theCdr = &cdr;
  return *c;
}

consCell& vec2list(gcVector& v, unsigned int n=0) {
  if (n==v.size()) return nil;
  else return cons(*v[n], vec2list(v,n+1));
}

void consCell::print(ostream &s) {
  s << '(' << car(this);
  clObject *o = this->theCdr;
  consCell *p;
  while((p = theCons(o)) && !null(p)) {
    s << ' ' << car(p);
    o = p->theCdr;
  }
  if (!null(o)) s << " . " << *o;
  s << ')';
}

clObject& consCell::operator[](int n) {
  return (n<=0) ? car(this) : theCons(cdr(this))[n-1];
}


int length(consCell& c) {
  return null(c) ? 0 : length(theCons(cdr(c)))+1;
}

int position(clObject& o, consCell& c, int i=0) {
  if (null(c)) return -1;
  return (&o == &(car(c)) ? i : position(o, theCons(cdr(c)), i+1));
}

gcVector& list2vec(consCell& c, gcVector& v = *new gcVector()) {
  //  gcVector& v = *new gcVector();
  clObject* cp = &c;
  while (cp->typeOf()=="cons") {
    v.push_back(&car(cp));
    cp = &cdr(cp);
  }
  return v;
}


////////////////////////////
///
///  Error
///
void error(string msg, clObject& arg) {
  throw &cons(*new clString(msg), arg);
}

clSymbol& intern(string s) {
  clSymbol* smb = symbolTable[s];
  if (smb == NULL) {
    smb = new clSymbol(s);
    symbolTable[s] = smb;
  }
  return *smb;
}

////////////////////////
///
///  Dynamic environments
///

void dEnv::print (ostream& s) {
  for (gc_map::iterator i = m.begin(); i != m.end(); i++ ) {
    if (i->second == this) continue;
    s << *i->first << " : ";
    if (i->second) s << *i->second;
    else s << "<unbound>";
    s << '\n';
  }
}

clObject& dEnv::lkup(clSymbol& s) {
  gc_map::iterator i = m.find(&s);
  if (i != m.end()) return *(i->second);
  if (!parent) { error("Unbound symbol", s); }
  return parent->lkup(s);
}


////////////////////////
///
///  Lexical environments
///
class Env : public gc {
public:
  gcVector& paramv;
  gcVector valuev;
  Env *parent;

  Env(clObject& params, gcVector& _paramv, gcVector& values, Env* _parent=0)
    : paramv(_paramv), valuev(values), parent(_parent)
  {
    if (valuev.size() != paramv.size())
      error("Wrong number of arguments", nil);
  }

  clObject& lkup(clSymbol& s) {
    int i = position(paramv, s);
    if (i>=0) return *valuev[i];
    if (parent) return parent->lkup(s);
    return TLE.lkup(s);
  }

  void set(clSymbol& s, clObject& o) {
    int i = position(paramv, s);
    if (i>=0) { valuev[i] = &o; return; }
    if (parent) { parent->set(s, o); return; }
    else TLE.set(s,o);
  }
};

// This really need to go somewhere else.  There are other static
// initializers in other files (e.g. primops) so we're playing Russion
// roullette here.
//
GC_initter dummy; // Force GC init before other static initializers
Env NLE(nil, *new gcVector(), *new gcVector());  // Null lexical environment


///////////////////////
//
// Closures
//

class clClosure : public clObject {
public:
  virtual clObject& apply(clVector& args) =0;
};

class vecClosure : public clClosure {
  Env& env;
  gcVector& params;
  gcVector& body;

  vecClosure(gcVector& _params, gcVector& _body, Env& _env)
    : env(_env), params(_params), body(_body) {}

  virtual clObject& apply(clVector& args) {
    Env& e = *new Env(nil, params, args.theVector, &env);
    gcVector::iterator start = body.begin()+1;
    gcVector::iterator end = body.end();
    clObject* result = &nil;
    for (gcVector::iterator i = start; i != end; i++) {
      result = &((**i).eval(e));
    }
    return *result;
  }    
};

class consClosure : public clClosure {
public:
  Env* env;
  clObject* params;  // The original formal parameters as specified by the user
  gcVector paramv;   // The formal parameters converted to a vector
  int restarg;       // Non-zero means the last formal is a restarg
  consCell* body;

  consClosure(clObject& _params, clObject& _body, Env& _env) {
    // Check for legal arglist
    try {
      paramv = dynamic_cast<clVector&>(_params).theVector;
    } catch(bad_cast) {
      clObject* p = &_params;
      consCell *p1;
      while ((p1 = theCons(p)) && !null(p1)) {
	if (!the<clSymbol>(&car(p1))) error("Illegal argument", car(p1));
	paramv.push_back(&car(p1));
	p = &cdr(p1);
      }
      if (!null(p)) {
	p = the<clSymbol>(p);
	if (!p) error("Illegal restarg", _params);
	paramv.push_back(p);
	restarg = (p ? 1 : 0);
      }
    }

    // Check for legal body
    clObject* p = &_body;
    consCell *p1;
    while ((p1 = theCons(p)) && !null(p1)) p = &cdr(p1);
    if (!null(p)) error("Illegal function body form", *p);
    
    params = &_params;
    body = theCons(&_body);
    env = &_env;
  }


  void print(ostream &os) { os << "cfn-" << *params << *body; }
  string typeOf() { return "consClosure"; }

  clObject& apply(clVector& args) {
    // Create a frame for the function arguments
    if (restarg) {
      int nargs = paramv.size();
      gcVector& restval = *new gcVector();
      for (Uint i = nargs-1; i<args.theVector.size(); i++)
	restval.push_back(&args[i]);
      clVector& v = *new clVector(restval);
      vset(args, nargs-1, v);
      args.theVector.resize(nargs);
    }
    Env& e = *new Env(*params, paramv, args.theVector, env);

    // Walk the body
    clObject *b = body;
    consCell *b1;
    clObject* result = &nil;
    while ((b1 = theCons(b)) && !null(b1)) {
      result = &(car(b1).eval(e));
      b = &cdr(b1);
    }
    return *result;
  }
};


/////////////////
///
///  Special forms
///

// sfApplier = pointer to function(argsT&, Env&) returning clObject&
typedef clObject&(*sfApplier)(argsT&, Env&);

class specialForm : public clObject {
public:
  string name;
  sfApplier applier;

  specialForm(const char* n, sfApplier a) : name(n), applier(a) {
    TLE.set(intern(name), *this);
  }

  void print(ostream &os) { os << "<Special form " << name << '>'; }
  string typeOf() { return "Special form"; }

  clObject& apply(argsT& args, Env& e) { return (*applier)(args, e); }
};

clObject& quote_apply(argsT& args, Env& e) { return args[0]; }

clObject& fn_apply(argsT& args, Env& e) {
  return *new consClosure(args[0], cdr(vec2list(args.theVector)), e);
}

clObject& set_apply(argsT& args, Env& e) {
  clSymbol& target = the<clSymbol>(args[0]);
  clObject& value = args[1].eval(e);
  e.set(target, value);
  return value;
}

clObject& if_apply(argsT& args, Env& e) {
  clObject& condition = args[0].eval(e);
  if (!null(condition)) return args[1].eval(e);
  return args[2].eval(e);
}

clObject& try_apply(argsT& args, Env& e) {
  try {
    return args[0].eval(e);
  } catch (clObject* o) {
    // Not quite right -- should evalute to a continuation and
    // pass *o as an arg.
    return args[1].eval(e);
  }
}

clObject& dotimes_apply(argsT& args, Env& e) {
  cl_I n = theInteger(args[0].eval(e));
  clObject& form = args[1];
  for (int i=0; i<n; i++) form.eval(e);
  return nil;
}

clObject& time_apply(argsT& args, Env& e) {
  stopwatch sw;
  sw.start();
  clObject& o = args[0].eval(e);
  cout << args[0] << " took " << sw.stop()*1000 << " msec.\n";
  return o;
}

//////////////////////
///
///  Unwind frames
///
class unwindFrame {
public:
  clObject *form;
  Env *e;

  unwindFrame(clObject &o, Env &env) { form = &o; e = &env; }
  // This ought to catch exceptions and do something reasonable
  ~unwindFrame() { form->eval(*e); }

private:
  unwindFrame();
  unwindFrame(const unwindFrame&);
};

clObject& unwind_apply(argsT& args, Env& e) {
  unwindFrame f(args[1], e);
  return args[0].eval(e);
}

////////////////////
///
/// Ctrl-C handler
///
#include <signal.h>
int stopeval = 0;
void ctrl_c_handler(int signum) { stopeval=1; }

//////////////
//
//  Stack guard
//
#define STACK_LIMIT 1000000
void* stackbottom = 0;


////////////////////////
///
///  Eval

clObject& eval(clObject& o) {
  return o.eval(NLE);
}

clObject& clSymbol::eval(Env& e) { return e.lkup(*this); }

clVector& evalArgs(argsT& args, Env &e) {
  clVector& result = *new clVector();
  for (gcVector::iterator i = args.theVector.begin();
       i != args.theVector.end();
       i++)
    vpe(result, (*i)->eval(e));
  return result;
}

clVector& evalArgs(consCell& args, Env &e) {
  clVector& result = *new clVector();
  clObject* p = &args;
  consCell *p1;
  while ((p1 = theCons(p)) && !null(p1)) {
    vpe(result, car(p1).eval(e));
    p = &cdr(p1);
  }
  return result;
}

void inline evalPrep() {
  // Check for ctrl-c
  if (stopeval) {
    stopeval=0;
    // insert debugger here some day
    throw &intern("Interrupt");
  }
  // Check for possible stack overflow
  int dummy;
  // Amazing that this doesn't generate a warning
  if (abs((long)stackbottom-(long)(&dummy)) > STACK_LIMIT) {
    throw &intern("Stack overflow");
  }
}

clObject& clVector::eval(Env& e) {
  evalPrep();
  argsT args;

  // Evaluate the operator
  clObject& op = (*this)[0].eval(e);

  // Special forms get unevaled arguments
  specialForm* sp = the<specialForm>(&op);
  if (sp) {
    for (Uint i=1; i < this->theVector.size(); i++)
      vpe(args, *(this->theVector[i]));
    return sp->apply(args, e);
  }

  // Primops and closures get evaluated arguments
  for (Uint i=1; i<this->theVector.size(); i++)
    vpe(args, (this->theVector[i])->eval(e));
  primop* p = the<primop>(&op);
  if (p) return p->apply(args);
  clClosure* c = dynamic_cast<clClosure*>(&op);
  if (c) return c->apply(args);
  // The operator was not callable
  throw &cons(intern("Not a function object"), op);
  return nil;
}

clObject& consCell::eval(Env& e) {
  evalPrep();
  // Evaluate the CAR  
  clObject& op = car(this).eval(e);
  // Special forms get unevaled arguments
  specialForm* sp = the<specialForm>(&op);
  if (sp) {
    argsT args(list2vec(theCons(cdr(this))));
    return sp->apply(args, e);
  }
  // Primops and closures get evaluated arguments
  argsT& args = evalArgs(theCons(cdr(this)), e);
  primop* p = the<primop>(&op);
  if (p) return p->apply(args);
  clClosure* c = dynamic_cast<clClosure*>(&op);
  if (c) return c->apply(args);
  // The CAR was not callable
  throw &cons(intern("Not a function object"), op);
  return nil;
}
  
void CL_init() { GC_init(); }

int main(int argc, const char* argv[]) {
  GC_INIT();
  new specialForm("quote", &quote_apply);
  new specialForm("set", &set_apply);
  new specialForm("fn", &fn_apply);
  new specialForm("if", &if_apply);
  new specialForm("try", &try_apply);
  new specialForm("dotimes", &dotimes_apply);
  new specialForm("time", &time_apply);
  new specialForm("unwind-protect", &unwind_apply);

  clSymbol& lastResult = intern("_");

  signal(SIGINT, &ctrl_c_handler);
  int dummy;
  stackbottom=&dummy;
  TLE.set(intern("tle"), TLE);

  while(1) {
    clObject& o = readObject();
    stopeval=0; // Clear latent interrupts
    try {
      clObject& result = eval(o);
      TLE.set(lastResult, result);
      cout << "--> " << result  << '\n';
    } catch (clObject *o) {
      cout << "*** Uncaught exception: " << *o << '\n';
    } catch (bad_cast) {
      cout << "*** Wrong type argument\n";
    } catch (bad_alloc) {
      cout << "*** Out of memory\n";
      exit(-1);
    } catch (out_of_range) {
      cout << "*** Vector reference out of range\n";
    } catch (...) {
      cout << "*** Zowie!  Weird exception thrown!\n";
    }
  }
  return 0;
}
