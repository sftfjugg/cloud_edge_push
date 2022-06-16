local cjson = require("cjson.safe")
local service_upload = require("service_upload")
local upload_info = ngx.shared.upload_info
local client_types = ngx.shared.tcp_client_types
local _M = {
    _VERSION = '0.01'
}

function _M.new( self )
    return setmetatable( {} , { __index = _M } )
end

function _M.run( ... )
    local result = {code = 0, message = "success"}
    local method = ngx.req.get_method()
    local data
    
    if method == "POST" then
        ngx.req.read_body()
        data = ngx.req.get_body_data() -- string
        local tbl = cjson.decode(data)  -- table
        local files_info = {}
        for _, t in ipairs(tbl) do
            if t.type and t.filename then
                table.insert(files_info, cjson.decode(upload_info:get(t.type .. '_' .. t.filename)))
            end
        end

        -- 推送至客户端
        for _, f in ipairs(files_info) do
            local sessionids = client_types:get_keys(0)
            local client_type
            for _, sessionid in ipairs(sessionids) do
                client_type = client_types:get(sessionid)
                if client_type == f.type then
                    service_upload.publish_message(f, sessionid)
                    ngx.log(ngx.INFO, "publish_message ".. cjson.encode(f))
                end
            end
        end

        local respone = cjson.encode(result)
        ngx.log(ngx.INFO, respone)
        ngx.say(respone)
    end
end
return _M

