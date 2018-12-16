
local t: { [string]:number|nil }

local a: { [number]:any }

t = a

t['horse'] = 34
t['horse'] = true
t[13] = 7
a[13] = 7

interface StrMap: {
    [string]: number
}

local sm: StrMap

interface Point2D: {
    x: number
    y: number
}

local d: Point2D = 42

d = { x=33, y=999 }
d = 42
d = { x=13 }

d.x = 45
d.x = nil
d.horse = 13
