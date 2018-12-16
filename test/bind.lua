
global select: (:string|number, ...): number|any
global table: {
    unpack: (: { [number]: any }, :number|nil, :number|nil): [...]
}

local function bind(func, ...)
    local n = select('#', ...)
    local bound = {...}
    return function (...)
        local args = {}
        local vn = select('#', ...)
        for i = 1, n do
            args[i] = bound[i]
        end
        for i = 1, vn do
            local e = select(i, ...)
            args[n + i] = e
        end
        return func(table.unpack(args, 1, n + vn))
    end
end

bind = nil
