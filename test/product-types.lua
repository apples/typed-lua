
global horse: ((x: number): string) & ((x: string): number)

horse = nil

local a = horse(42)
local b = horse('asdf')

a = nil
b = nil

local horse2: ((x: number): string) & ((x: string): number)

horse2 = horse
