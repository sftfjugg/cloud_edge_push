rawset(_G, 'lfs', false)
local route = require("resty.route").new()

route {
    ["=/sys/v1/service_upload"] = function()  
        local srv = require("service_upload").new()
        srv.run()
    end,
    ["=/sys/v1/service_api"] = function()  
        local srv = require("service_api").new()
        srv.run()
    end
}

route:dispatch()

ngx.log(ngx.NOTICE, "end .........")