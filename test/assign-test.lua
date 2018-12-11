
local x: number

x = function ():number end

local function f():number end

x = f()
x = f

local a: number, b: number, c: number

a, b, c = 1, 2, 3
a, b = 1, 2, 3
a, b, c = 1, 2
