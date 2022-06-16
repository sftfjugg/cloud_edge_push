local cjson = require("cjson.safe")
local upload_info = ngx.shared.upload_info
local _M = {
    _VERSION = '0.01'
}

function _M.new( self )
    return setmetatable( {} , { __index = _M } )
end

function _M.run( ... )
    local result = {code = 0, message = "success"}
    local method = ngx.req.get_method()
    local param
    
    if method == "GET" then
        param = ngx.req.get_uri_args() --[[  table ]] 
        if param and next(param) then
            if param.filename and param.type then
                local data = upload_info:get(param.type .. '_' .. param.filename)
                result.data = cjson.decode(data)
            else
                ngx.say('param error, need type and filename!')
                return
            end
        else

            local file_infos = upload_info:get_keys(0)
            local tbl = {}
            for _, f_key in ipairs(file_infos) do
                local f = upload_info:get(f_key)
                table.insert(tbl, cjson.decode(f))
            end
            result.data = tbl
        end
    end
    local respone = cjson.encode(result)
    ngx.log(ngx.INFO, respone)
    ngx.say(respone)
end

return _M

