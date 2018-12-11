local function func(): [a: number, b: string]
    return 7, "asdf"
end

local x: number, y: number

x, y = func()

x = func()
