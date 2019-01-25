
local identity: <T>(x: T): T

identity = nil

local a = identity(42)
local b = identity('horse')

a = nil
b = nil

b = 42

local ipairs: <V, T: {[number]: V}>(t: T): [:(:T, :number):[:number, :V], :T, :number]

local t: {[number]: 1|2|3} = {1, 2, 3}

local x, y, z = ipairs(t)

x = nil
y = nil
z = nil

function ipairs<V, T: {[number]: V}>(t: T): [:(:T, :number):[:number, :V], :T, :number]
    local function iter(t: T, i: number)
        return i+1, t[i]
    end

    return iter, t, 0
end

local obj = {}

global print: (:any):void

function obj:horse<T>(x: T): T
    print(t)
    return t
end

local h = obj.horse

h = nil

obj.horse(obj, 42)

local x = obj:horse(42)

x = nil

local function ipairs_local<V, T: {[number]: V}>(t: T): [:(:T, :number):[:number, :V], :T, :number]
    local function iter(t: T, i: number)
        return i+1, t[i]
    end

    return iter, t, 0
end

local xl, yl, zl = ipairs_local(t)

xl = nil
yl = nil
zl = nil
