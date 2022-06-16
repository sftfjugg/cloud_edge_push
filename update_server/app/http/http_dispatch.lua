rawset(_G, 'lfs', false)
local route = require("resty.route").new()

route {
    ["=/sys/v1/service_upload"] = function()  -- 上传文件
        local srv = require("service_upload").new()
        srv.run()
    end,
    ["=/sys/v1/service_query"] = function()  -- 已上传文件查询接口
        local srv = require("service_query").new()
        srv.run()
    end,
    ["=/sys/v1/service_pushfile"] = function()  -- 下发文件给指定客户端
        local srv = require("service_pushfile").new()
        srv.run()
    end
}

route:dispatch()

ngx.log(ngx.NOTICE, "end .........")