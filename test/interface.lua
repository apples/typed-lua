
interface Horse: number|string

local x: Horse = 42

x = 7

x = 'asdf'

x = nil

interface CrazyFunc: (): CrazyFunc

local function crazy(): CrazyFunc
    return crazy
end

crazy()()()()()()()()()
