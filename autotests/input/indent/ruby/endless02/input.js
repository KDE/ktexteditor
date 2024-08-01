v.setCursorPosition(3, 25);
v.enter();
v.enter();

// basic method definitions
v.type("def self.call(...) = new(...).call");
v.enter();
v.enter();

v.type("def call = raise NotImplementedError");
v.enter();
v.enter();

v.type("def call() = :endless");
v.enter();
v.enter();

v.type("def compact? =true");
v.enter();
v.enter();

// default parameters
v.type("def expand   (  ts = Time.now )    = ts.year * (rand * 1000)")
v.enter();
v.enter();

v.type("def foo(bar = \"baz\")");
v.enter();
v.type("bar.upcase");
v.enter();
v.type("end");
v.enter();
v.enter();

v.type("def foo_blank bar = \"baz\"");
v.enter();
v.type("bar.upcase");
v.enter();
v.type("end");
v.enter();
v.enter();

// default parameters, but endless (parenthesis are required around the params now)
v.type("def foo_endless(bar = \"baz\") = bar.upcase");
v.enter();
v.enter();

// setter methods can't be endless, make sure the `=` of the method name doesn't get picked up as such
v.type("def width=(other)")
v.enter();
v.type("@width = other");
v.enter();
v.type("end");
v.enter();
v.enter();

v.type("def height= other")
v.enter();
v.type("@height = other");
v.enter();
v.type("end");
v.enter();
