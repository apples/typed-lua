
local f: (a: number, b: string): void

f(7, 13)

f('asdf', 'qwer')

f(42, 'piou')

local fv: (a: number, ...): void

fv(7)

fv(7, 1, 2, 3)

fv('horse', 1, 2, 3)

interface T: {
    m: (self: T, x: number): void
}

local t: T

t:m(42)

t:m('horse')
