
local x = 42

x = 'horse'

local t = {}

t = 42

local function func()
    return 1, 'horse', { fld=42 }
end

local a, b, c, d = func()

a = nil
b = nil
c = nil
d = nil
