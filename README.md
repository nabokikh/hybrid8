# Hybrid8, aka H8

_Warning_ this gem has yet limited functionality and is under main development. Versions suitable
for an open beta test will start from 0.1.*.

Therubyracer gem alternative to effectively work with ruby 2.1+ in multithreaded environment in an
effective and GC-safe way. Should be out of the box replacement for most scenarios.

Special features:

- care about wrapped Ruby objects lifespan (are kept until either ruby or javascript context
reference wrapped object). In no way GC will not reclaim ruby object that is in use by the
javascript context

- care about wrapped Javascript objects lifetime the same. Referenced JS items will not be recycled
while there are ruby objects referencing it. It also means that once created H8::Context will not
be reclaimed as long as there is at least one active wrapped object returned from the script.

- you can pass ruby objects from ruby code called from javascript back to the ruby code intact.
Ruby objects are automatically wrapped in js code and unwrapped in ruby code (you might need to
call #to_ruby)

- Uncaught ruby exceptions are thrown as javascript exceptions in javascript code. The same,
uncaught javascript exceptions raise ruby error in ruby code.

## Main difference from therubyracer

- correct and accurate object tracking in both JS and Ruby VMs, GC aware.

- passed thru objects and exceptions in js -> ruby -> js -> ruby chains are usually kept unchanged,
e.g. wrapped in one language then unwrapped when passed to the original language

- Not Yet: source information in uncaught exception in js

- Script is executed in the calling ruby thread without unblocking it (for the sake of
effectiveness). If we would release GIL and reacquire it, it would take more time. And there is no
multithreading support yet (this one might be added soon).


## Installation

### Prerequisites

You should have installed libv8, use latest version with v8::Isolate and v8::Locker. This version
may not find you installation, contact me if you have problems, I'll tune it up.

#### Macos (10.9 maybe+)

The working process:

* install from sources 3.31.77 (or try newer)

    gclient update
    export CXXFLAGS='-std=c++11 -stdlib=libc++ -mmacosx-version-min=10.9'
    export LDFLAGS=-lc++
    make native

Note that exporting symbols is a hack that may not be in need anymore.

### Setting up

Add this line to your application's Gemfile:

    gem 'hybrid8'

And then execute:

    $ bundle

Or install it yourself as:

    $ gem install hybrid8

## Usage

Is generally like therubyracer gem. Create context, set variables, run scripts.

    require 'h8'

    res = H8::Context.eval "({foo: 'hello', bar: 'world'});"
    puts "#{res.foo} #{res.bar}"

another way to access attributes:

    puts res['foo']

The same works with arrays:

    res = H8::Context.eval "['foo', bar'];"
    puts res[1]

To set context variables:

    cxt = H8::Context.new some: 'value'
    cxt[:pi] = 3.1415

You can return function and call it from ruby:

    fun = cxt.eval "(function pi_n(n) { return pi * n; })"
    p fun(2)

The same you can return objects and call its member functions - if a member is a function,
it will be called with given arguments:

    res = H8::Context.eval <<-End
      function cls(base) {
        this.base = base;
        this.someVal = 'hello!';
        this.noArgs = function() { return 'world!'};
        this.doAdd = function(a, b) {
          return a + b + base;
        }
      }
      new cls(100);
    End
    res.someVal.should == 'hello!'
    res.noArgs.should == 'world!'
    res.doAdd(10, 1).should == 111

## Contributing

Note that at this early point of development you better first talk to me to not to reinvent the
wheel.

1. Fork it ( https://github.com/[my-github-username]/hybrid8/fork )
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create a new Pull Request


