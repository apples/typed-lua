
local function f(): [:number, :string, ...]
    return 7, "asdf"
end

local x: number, y: number

x, y = f()
