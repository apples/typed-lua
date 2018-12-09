
asdf = 7

global horse

horse = 42

local x = 7

local function func(a, b)
    local y = a
    return x + y + b
end

local function nodots()
    global print
    print(...)
end

local function dots(...)
    print(...)
end

global print

for x=1,2,3 do
    print(x)
end

global pairs
local x

for k,v in pairs(x) do
    print(k, v)
end
