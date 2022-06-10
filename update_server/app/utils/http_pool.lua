local http = require("resty.http")

local _M = {
    _VERSION = "0.1.0"
}

function _M:new()
    return setmetatable({}, {__index = _M})
end

local DEFAULT_HTTP_NAME = "http_client"

function _M:get_connect(name, host, port, timeout)

    local timeout = timeout or 500
    local name  = name or DEFAULT_HTTP_NAME

    if name and ngx.ctx[name] then
      return ngx.ctx[name]
    end
    if not host or not port then
        return
    end

    local httpc, ok, err
    httpc = http:new()
    httpc:set_timeout(timeout)

    
    local ok, err = httpc:connect(host, port)
    if not ok then
        return nil, err
    end


    ngx.ctx[name] = httpc

    return httpc

  end

function _M:close(name, keepalive_time, pool_size)
    local keepalive_time = keepalive_time or 60000
    local pool_size = pool_size or 100
    local name = name or DEFAULT_HTTP_NAME

    if ngx.ctx[name] then
      ngx.ctx[name]:set_keepalive(keepalive_time, pool_size)
      ngx.ctx[name] = nil
    end
end

return _M
