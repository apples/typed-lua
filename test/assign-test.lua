
local x: number

x = function ():number end

local function f():number end

x = f()
x = f

local a: number, b: number, c: number

a, b, c = 1, 2, 3
a, b = 1, 2, 3
a, b, c = 1, 2

local ns: number|string

ns = 7
ns = 'asdf'
ns = nil

local sb: string|boolean

ns = sb
sb = ns

local nsb: number|string|boolean

nsb = ns
nsb = sb
ns = nsb
