local cjson = require("cjson.safe")
local unqlite = require('unqlite')
local def = require('com_def')
local util = require("utils.utility")
local upload_info = ngx.shared.upload_info

-- 保证定时器只初始化一次
-- 从upload.db加载数据到共享内存
if 0 == ngx.worker.id() then
    upload_info:flush_all()
    if util.fileExists(def.updatedb) then
        local db = unqlite.open(def.updatedb)
        local cur = db:cursor()
        assert(cur:first())
        while true do
            upload_info:set(cur:key(), cur:data())
            if not cur:next_entry() then
                break
            end
        end
        assert(cur:release())
        db:close()

        -- for debug--
        local file_infos = upload_info:get_keys(0)
        local tbl = {}
        for _, f_key in ipairs(file_infos) do
            local f = upload_info:get(f_key)
            table.insert(tbl, cjson.decode(f))
        end
        ngx.log(ngx.INFO, cjson.encode(tbl))
    end
end