module H8
  class Context
    # Create new context optionally providing variables hash
    def initialize timout: nil, **kwargs
      set_all **kwargs
    end

    # set variables from keyword arguments to this context
    def set_all **kwargs
      kwargs.each { |name, value|
        set_var(name.to_s, value)
      }
    end

    # Set variable for the context
    def []= name, value
      set_all name => value
    end

    # Execute a given script on the current context with optionally limited execution time.
    #
    # @param [Float] timeout if is not 0 then maximum execution time in seconds
    # @return [Value] wrapped object returned by the script
    # @raise [H8::TimeoutError] if the timeout was set and expired
    def eval script, timeout: 0
      _eval script, (timeout*1000).to_i
    end

    # Execute script in a new context with optionally set vars. @see H8#set_all
    # @return [Value] wrapped object returned by the script
    def self.eval script, **kwargs
      Context.new(** kwargs).eval script
    end

    # Secure gate for JS to securely access ruby class properties (methods with no args)
    # and methods. This class implements security policy! Overriding this method could
    # breach security and provide full access to ruby object trees.
    #
    # It has very complex logic so the security model update should be done somehow
    # else.
    def self.secure_call instance, method, args=nil
      method = method.to_sym
      begin
        m = instance.public_method(method)
        if m.owner == instance.class
          return m.call(*args) if method[0] == '[' || method[-1] == '='
          if m.arity != 0
            return -> (*args) { m.call *args }
          else
            return m.call *args
          end
        end
      rescue NameError
        # No exact method, calling []/[]= if any
        method, args = if method[-1] == '='
                         [:[]=, [method[0..-2].to_s, args[0]] ]
                       else
                         [:[], [method.to_s]]
                       end
        begin
          m = instance.public_method(method)
          if m.owner == instance.class
            return m.call(*args)
          end
        rescue NameError
          # It means there is no [] or []=, e.g. undefined
        end
      end
      H8::Undefined
    end

    private

  end
end
