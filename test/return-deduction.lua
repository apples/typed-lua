
local function func( is_horse: boolean )
    if is_horse then
        return 7
    else
        return 'horse'
    end
end

func = nil

func = function ( is_dorse: boolean|string )
    return 42
end

local function func2( is_dorse: boolean|string )
    return 42
end

func2 = nil

func2 = function ( is_horse: boolean )
    if is_horse then
        return 7
    else
        return 'horse'
    end
end

func = func2

func2 = func
