local cjson = require("cjson.safe")
local unqlite = require('unqlite')
local def = require('com_def')
local _M = {
    _VERSION = '0.01'
}

function _M.new( self )
    return setmetatable( {} , { __index = _M } )
end

function _M.run( ... )
    local result = {code = 0, message = "success"}
    local method = ngx.req.get_method()
    local  param
    
    if method == "GET" then
        local headers = ngx.req.get_headers()
        param = ngx.req.get_uri_args() --[[  table ]] 
        if param and next(param) then
            if param.filename and param.type then
                local db = unqlite.open(def.updatedb)
                local data = db:get(param.type .. '_' .. param.filename)
                db:close()
                result.data = cjson.decode(data)
            else
                ngx.say('param error, need type and filename!')
                return
            end
        else
            local db = unqlite.open(def.updatedb)
            local cur = db:cursor()
            assert(cur:first())
            local tbl = {}
            local index = 1
            while true do
                local d = cur:data()
                tbl[index] = cjson.decode(d)
                index = index + 1
                if not cur:next_entry() then
                    break
                end
            end
            assert(cur:release())
            db:close()
            result.data = tbl
        end
    end
    if result.data then
        local respone = cjson.encode(result)
        ngx.log(ngx.INFO, respone)
        ngx.say(respone)
    end
end

return _M

