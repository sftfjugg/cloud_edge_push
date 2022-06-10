local cjson = require("cjson.safe")
local util = require("utils.utility")
local post = require('resty.post')
local zzlib = require("zzlib.zzlib")
local unqlite = require('unqlite')
local def = require('com_def')
local lfs   = require("lfs")

local notify_msg_queue = ngx.shared.notify_msg_queue
local client_types = ngx.shared.tcp_client_types

local _M = {
    _VERSION = '0.0.1'
}

function _M.new(self)
    return setmetatable({}, { __index = _M })
end

-- 获取解压后文件的名称 --
local function decompress(src_path)
    
    ngx.log(ngx.INFO, "list zip file start " .. os.time())
    local file_r = io.open(src_path, "rb")
    if not file_r then
        return nil
    end
    local file_content = {}
    local input = file_r:read("*all")
    local file_list = zzlib.filelist(input)
    for _, file_path in ipairs(file_list) do
        table.insert(file_content, file_path)
    end
    file_r:close()
    ngx.log(ngx.INFO, "list zip file end " .. os.time())
    return file_content
end

local function publish_message(tbl, sessionid)

    if not tbl then
        ngx.log(ngx.ERR, "parameter error when publishing to client process")
        return
    end

    local msg = cjson.encode({ data = tbl, timestamp =  ngx.now() * 1000})
    local rec, err = notify_msg_queue:rpush(sessionid, msg) -- 添加到每个session的消息队列中
    if not rec then
        ngx.log(ngx.ERR, 'push msg to queue fail!msg: ', msg, ", error: ", err)
    end
end

local function update_fileinfo(f)

    local extension = util.getExtension(f.filename)
    local zip_info = nil            -- 非压缩文件，不会有zip_info
    f.md5 = util.md5(f.remotefile_url)    -- 设置文件md5值
    local res = lfs.attributes(f.remotefile_url)
    f.modify_time = res.modification
    f.filesize = res.size

    if extension == "zip" then
        local extract_content = decompress(f.remotefile_url)
        zip_info = cjson.encode(extract_content)
    end
    f.zip_info = zip_info
end



local function mkdir(p)
    if util.fileExists(p) ~= nil then
        os.execute("mkdir -p " .. p)
    end
end


local function print_allfiles()
    local db = unqlite.open(def.updatedb)
    local cur = db:cursor()
    assert(cur:first())
    while true do
        local k = cur:key()
        assert(k)
        local d = cur:data()
        if not cur:next_entry() then
            break
        end
    end
    assert(cur:release())
    db:close()
end


function _M.run( ... )
    local result = {code = 0, message = "success"}

    if util.fileExists(def.tmp_root) ~= nil then
        os.execute("mkdir -p " .. def.tmp_root)
    end

    if util.fileExists(def.download_root) ~= nil then
        os.execute("mkdir -p " .. def.download_root)
    end

    local post = post:new({ path = def.tmp_root })
    local m = post:read()

    ngx.log(ngx.INFO, util.dump(m))
    m.files_info = util.trim(m.files_info)
    local files_info = cjson.decode(m.files_info)
    for _, f in ipairs(files_info) do
        local v = m.files[f.filename]
        if v then
            local filepath = def.download_root
            if f.type then
                filepath = filepath .. f.type .. "/"
            end
            mkdir(filepath)
            f.remotefile_url = filepath .. f.filename
            if not f.save_path then
                f.save_path = f.remotefile_url
            end
            os.rename(def.tmp_root .. v.tmp_name, f.remotefile_url)
            update_fileinfo(f)

            -- 添加上传记录进db --
            local db = unqlite.open(def.updatedb)
            db:set(f.type .. '_' .. f.filename, cjson.encode(f))
            db:close()
        end
    end

    for _, f in ipairs(files_info) do
        local sessionids = client_types:get_keys(0)
        local client_type
        for _, sessionid in ipairs(sessionids) do
            client_type = client_types:get(sessionid)
            if client_type == f.type then
                publish_message(f, sessionid)
            end
        end
    end
    -- print_allfiles()
    result.data = files_info
    ngx.say(cjson.encode(result))
end

return _M