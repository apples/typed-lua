
local nums = {1, 2, 3}

nums = nil

local x = table.remove(nums)

x = nil

x = table.remove(nums, 5)

table.sort(nums, function (a: number, b: number) return a < b end)
