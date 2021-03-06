#ifndef __js_gate_h
#define __js_gate_h

#include "h8.h"
#include "allocated_resource.h"

using namespace v8;

namespace h8 {

/**
 * Interface to anything that could be converted to a Javascipt object. Provides common helpers.
 */
class JsValue: public AllocatedResource {
public:
	virtual Local<Value> value() const = 0;

	Local<Object> object() const {
		return value()->ToObject();
	}

	virtual Isolate* isolate() = 0;
	virtual ~JsValue() {
	}
};

/**
 * Gates JS object to ruby environment. Holds persistent reference to the source js object until
 * ruby object is recycled (then frees it). Note that this ruby object is not meant to be kept alive
 * by the H8 instance, instead, its owner should.
 *
 * Methods of this class do not need the H8::Scope, they create one internally.
 */
class JsGate: public JsValue {
public:
	/**
	 * Used in the ruby allocator. Do not call unless you know what you do.
	 */
	JsGate() {
	}

	/**
	 * Return Ruby object that gates specified Handled javascript object. Ruby object
	 * locks permanently value until get recycled.
	 */
	template<class T>
	static VALUE to_ruby(H8* h8, const Handle<T>& value);

	/**
	 * Reset gate to the specified handle.
	 */
	template<class T>
	void set(H8 *h8, const Handle<T>& val) {
		this->h8 = h8;
		persistent_value.Reset(h8->getIsolate(), val);
		h8->add_resource(this);
	}

	/**
	 * Get ruby string representation
	 */
	VALUE to_s() {
		H8::Scope scope(h8);
		String::Utf8Value res(value());
		return *res ? rb_str_new2(*res) : Qnil;
	}

	/**
	 * Get ruby integer representation (FIXNUM)
	 */
	VALUE to_i() {
		H8::Scope scope(h8);
		return INT2FIX(value()->IntegerValue());
	}

	/**
	 * Get ruby Float representation (FIXNUM)
	 */
	VALUE to_f() {
		H8::Scope scope(h8);
		return DBL2NUM(value()->NumberValue());
	}

	/**
	 * @return true if the object is a primitive integer
	 */
	VALUE is_int() {
		H8::Scope scope(h8);
		return value()->IsInt32() ? Qtrue : Qfalse;
	}

	/**
	 * @return true if the object is a primitive float
	 */
	VALUE is_float() {
		H8::Scope scope(h8);
		return value()->IsNumber() ? Qtrue : Qfalse;
	}

	/**
	 * @return true if the object is an array
	 */
	VALUE is_array() {
		H8::Scope scope(h8);
		return value()->IsArray() ? Qtrue : Qfalse;
	}

	/**
	 * @return true if the object is an object
	 */
	VALUE is_object() {
		H8::Scope scope(h8);
		return value()->IsObject() ? Qtrue : Qfalse;
	}

	/**
	 * Retreive JS object attribute and convert it to the ruby wrapper
	 * of the new JsGate instace.
	 */
	VALUE get_attribute(VALUE name) {
		H8::Scope scope(h8);
		Local<Value> v8_name = v8::String::NewFromUtf8(isolate(),
				StringValueCStr(name));
		return h8->to_ruby(object()->Get(v8_name));
	}

	/**
	 * Set JS object attribute to ruby value
	 *
	 */
	void set_attribute(VALUE name, VALUE value) {
		H8::Scope scope(h8);
		Local<Value> v8_name = v8::String::NewFromUtf8(isolate(),
				StringValueCStr(name));
		object()->Set(v8_name, h8->to_js(value));
	}

	/**
	 * Access indexed property from ruby environment
	 */
	VALUE get_index(VALUE index) {
		H8::Scope scope(h8);
		return h8->to_ruby(object()->Get(NUM2INT(index)));
	}

	/**
	 * @return true if the object is a primitive string
	 */
	VALUE is_string() {
		H8::Scope scope(h8);
		return value()->IsString() ? Qtrue : Qfalse;
	}

	/**
	 * True if the wrapped object is a function
	 */
	VALUE is_function() {
		H8::Scope scope(h8);
		return value()->IsFunction();
	}

	/**
	 * Usually unneeded function, as H8 converts undefined values to
	 * H8::Undefined
	 */
	VALUE is_undefined() {
		H8::Scope scope(h8);
		return value()->IsUndefined() ? Qtrue : Qfalse;
	}

	/**
	 * Call this as function over global context
	 */
	VALUE call(VALUE args) const {
		H8::Scope scope(h8);
		return apply(h8->getContext()->Global(), args);
	}

	/**
	 * apply wrapped function to a given 'this' value and arguments
	 * wrapped in the ruby array
	 */
	VALUE apply(VALUE self, VALUE args) const {
		H8::Scope scope(h8);
		return apply(h8->gateObject(self), args);
	}

	/**
	 * @return bound ruby H8::Context instance
	 */
	VALUE ruby_context() const {
		return h8->ruby_context();
	}

	/**
	 * Call this object as function applying it to the object self
	 * (which will be this during javascript call)
	 */
	VALUE apply(Local<Value> self, VALUE args) const;

	virtual void free() {
		persistent_value.Reset();
		AllocatedResource::free();
		h8 = 0;
		// It is used no more, and it will be GC'd by ruby, until then
		// we can not delete it!
	}

	virtual void rb_mark_gc() {
		rb_gc_mark(h8->ruby_context());
	}

	virtual Local<Value> value() const {
		return Local<Value>::New(h8->getIsolate(), persistent_value);
	}

	virtual Isolate* isolate() {
		return h8->getIsolate();
	}

	virtual ~JsGate() {
		if( h8 ) {
			Locker l(h8->getIsolate());
//			puts("~JsGate1");
			persistent_value.Reset();
		}
		else {
//			puts("~JsGate2");
			persistent_value.Reset();
		}
	}

private:
	friend void rvalue_mark(void* ptr);
	friend class H8;

	H8 *h8 = 0;
	Persistent<Value> persistent_value;
};

}

#include "ruby_gate.h"
#include <ruby/encoding.h>

template<class T>
VALUE h8::JsGate::to_ruby(H8* h8, const Handle<T>& value) {
	// Convert primitives
	Local<Value> v = value;
	if (v->IsString()) {
		H8::Scope scope(h8);
		String::Utf8Value res(v);
		if( *res )
			return rb_enc_str_new(*res, res.length(), rb_utf8_encoding());
		return Qnil;
	}
	if (v->IsInt32()) {
		return INT2FIX(v->Int32Value());
	}
	if (v->IsNumber()) {
		return DBL2NUM(v->NumberValue());
	}
	if( v->IsUndefined()) {
		return Rundefined;
	}
	if( v->IsTrue() ) {
		return Qtrue;
	}
	if( v->IsFalse() ) {
		return Qfalse;
	}
	if( v->IsNull() ) {
		return Qnil;
	}
	RubyGate *rg = RubyGate::unwrap(v.As<v8::Object>());
	if( rg ) {
		return rg->rubyObject();
	}
	JsGate *gate;
	VALUE ruby_gate = rb_class_new_instance(0, NULL, value_class);
	Data_Get_Struct(ruby_gate, JsGate, gate);
	gate->set(h8, value);
	return ruby_gate;
}


#endif
