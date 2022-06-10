
local _M = {
    _VERSION = '0.01'
}

local function __dump(v, t, p)    
    local k = p or "";

    if type(v) ~= "table" then
        table.insert(t, k .. " : " .. tostring(v));
    else
        for key, value in pairs(v) do
            __dump(value, t, k .. "[" .. key .. "]");
        end
    end
end

-- get file extension
function _M.getExtension(str)
    return str:match(".+%.(%w+)$")
end

-- Convert table to string to help printing and debugging.
function _M.dump(v)
    local t = {'\n'};
    __dump(v, t);
    return table.concat(t, "\n->> ") .. '\n'
end

-- Remove spaces around the word
function _M.trim(s)
    return (ngx.re.gsub(s, "^\\s*(.*?)\\s*$", "$1", "jo"))
end


-- source: https://github.com/bigplum/lua-resty-mongol/issues/33
local function _convertHexStringToNormal(str)
    return (str:gsub(
        "..",
        function(cc)
            return string.char(tonumber(cc, 16))
        end
    ))
end


-- 将数字字符串转换为数字 --
function _M.convert_string_to_number(num)
    local number
    local convert = function()
        number = tonumber(num)
    end
    if num then
        pcall(convert)
    end
    return number
end

local function _positive_integer(num)
    local pattern = '^[1-9][0-9]*$'
    return string.find(num, pattern)
end


-- 将yyyy-MM-dd HH:mm:ss时间格式的字符串装换成时间戳
function _M.convert_string_to_timestamp(str)
    local y, mon, d, h, min, sec
    y = string.sub(str, 1, 4)
    mon = string.sub(str, 6, 7)
    d = string.sub(str, 9, 10)
    h = string.sub(str, 12, 13)
    min = string.sub(str, 15, 16)
    sec = string.sub(str, 18, 19)
    local timestamp = os.time({year = y, month = mon, day = d, hour = h, min = min, sec = sec}) * 1000

    return timestamp
end

-- 将字符串转换成Unicode格式
function _M.convert_string_to_unicode(str)
    local tag = '\\u'
    local step = 4
    local index = 0
    local a, b
    local s = ''

    for i=1, #str, 4 do
        a = string.sub(str, i,i +1)
        b = string.sub(str, i + 2,i + 3)
        s = s .. tag .. b .. a
    end

    return s
end

-- string
function _M.fromhex(str)
    return (str:gsub('..', function (cc)
        return string.char(tonumber(cc, 16))
    end))
end

function _M.tohex(str)
    return (str:gsub('.', function (c)
        return string.format('%02X', string.byte(c))
    end))
end


function _M.split( str, delimiter )
    if str == nil or str=='' or delimiter == nil then
        return nil
    end
    local result = {}
    for match in (str..delimiter):gmatch("(.-)"..delimiter) do
        result[#result+1] = match
    end
    return result
end

function _M.covert_string_to_table(str)
    local t = {}
    str:gsub(".", function(c) table.insert(t, c) end)
    
    return t
end

function _M.sessionid(ip, port)
    return table.concat({"client", ngx.re.gsub(ip, "\\.", "_", "jo"), port}, "_")
end

function _M.convert_address_to_sessionid(address)
    return "client".."_"..ngx.re.gsub(address, "\\.", "_", "jo")
end

-- xxx.txt return xxx
function _M.getfilename(str)
    if str == nil or str=='' then
        return nil
    end
    local idx = str:match(".+()%.%w+$")
    if(idx) then
        return str:sub(1, idx-1)
    else
        return str
    end
end

function  _M.fileExists(path)
    local file = io.open(path, "rb")
    if file then file:close() end
    return file ~= nil
end

-- calculate file md5 --
function _M.md5(path)
    if not path and '' == path then
      ngx.log(ngx.ERR, "file path illegality when computing MD5")
      return
    end
    local file = io.open(path, "rb")
    local file_md5
    if file then
      local file_string = file:read("*a") 
      file_md5 = ngx.md5(file_string)
      file:close()
      file = nil
    else
      ngx.log(ngx.ERR, "unable to read a file when calculating MD5")
    end
    return file_md5
end


return _M
