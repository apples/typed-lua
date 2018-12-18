
local x = {}

x.a = 7

x.a = 'horse'

x.b = nil

function x:method(bbb: number)
    return self.a
end

function x:call_method()
    return self:method(42)
end

x = nil
