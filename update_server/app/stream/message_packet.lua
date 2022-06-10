--[[
功能：消息的组包和分包，以及读取数据；
消息格式在下面代码中找注释
ref: https://github.com/openresty/lua-resty-mysql/blob/master/lib/resty/mysql.lua
]]--

local bit = require("bit")
local band = bit.band
local bxor = bit.bxor
local bor = bit.bor
local lshift = bit.lshift
local rshift = bit.rshift
local tohex = bit.tohex

local ok, new_tab = pcall(require, "table.new")
if not ok then
    new_tab = function (narr, nrec) return {} end
end

local _M = {
    _VERSION = '0.01'
}

local mt = { __index = _M }


local function _get_byte2(data, i)
    local a, b = string.byte(data, i, i + 1)
    return bor(a, lshift(b, 8)), i + 2
end


local function _get_byte3(data, i)
    local a, b, c = string.byte(data, i, i + 2)
    return bor(a, lshift(b, 8), lshift(c, 16)), i + 3
end


local function _get_byte4(data, i)
    local a, b, c, d = string.byte(data, i, i + 3)
    return bor(a, lshift(b, 8), lshift(c, 16), lshift(d, 24)), i + 4
end


local function _get_byte8(data, i)
    local a, b, c, d, e, f, g, h = string.byte(data, i, i + 7)

    -- XXX workaround for the lack of 64-bit support in bitop:
    local lo = bor(a, lshift(b, 8), lshift(c, 16), lshift(d, 24))
    local hi = bor(e, lshift(f, 8), lshift(g, 16), lshift(h, 24))
    return lo + hi * 4294967296, i + 8

    -- return bor(a, lshift(b, 8), lshift(c, 16), lshift(d, 24), lshift(e, 32),
               -- lshift(f, 40), lshift(g, 48), lshift(h, 56)), i + 8
end


local function _set_byte2(n)
    return string.char(band(n, 0xff), band(rshift(n, 8), 0xff))
end


local function _set_byte3(n)
    return string.char(band(n, 0xff),
                   band(rshift(n, 8), 0xff),
                   band(rshift(n, 16), 0xff))
end


local function _set_byte4(n)
    return string.char(band(n, 0xff),
                   band(rshift(n, 8), 0xff),
                   band(rshift(n, 16), 0xff),
                   band(rshift(n, 24), 0xff))
end


local function _from_cstring(data, i)
    local last = string.find(data, "\0", i, true)
    if not last then
        return nil, nil
    end

    return string.sub(data, i, last), last + 1
end


local function _to_cstring(data)
    return data .. "\0"
end


local function _to_binary_coded_string(data)
    return string.char(#data) .. data
end


local function _dump(data)
    local len = #data
    local bytes = new_tab(len, 0)
    for i = 1, len do
        bytes[i] = string.format("%x", string.byte(data, i))
    end
    return table.concat(bytes, " ")
end

local function _dumphex(data)
    local len = #data
    local bytes = new_tab(len, 0)
    for i = 1, len do
        bytes[i] = tohex(string.byte(data, i), 2)
    end
    return table.concat(bytes, " ")
end

-----------------------------------------------------------

--[[
    params: self 
            , sync_bytes -- 同步字节
            , typ --------- 消息类型(整数)
            , body -------- encode过的json字符串
    return: bytes(len of send content), err
    desc: 使用传来的参数构建一个TCP消息文档中定义的消息结构，并且在发送消息前申请一个锁，防止同一时间
          对数据库类型的TCP连接发送过多的请求导致数据库崩溃。发送完成后恢复锁
]]--
local MAX_MESSAGE_SIZE = 1024 * 1024 * 64;
local TOTAL_HEAD_LEN = 16   -- 头部总长16bytes

function _M.packet_message(sync_bytes, typ, body)
    local message, full_len
    local bodylen = #body

    message = _set_byte4(typ) .. _set_byte4(bodylen) ..  body 
    full_len = bodylen + TOTAL_HEAD_LEN

    message = _set_byte4(sync_bytes) .. _set_byte4(full_len)  .. message

    return message
end

--[[
    params: self
    return: , sync_bytes -- 同步字节
            , full_len ---- 全部长度
            , typ --------- 消息类型(整数)
            , body -------- 字符串通常为json或xml的序列化字符串
            , binarydata -- 字节型数据，通常是图片
            , err --------- 如果错误返回错误信息
    desc: receive raw data from TCP and parse it to needed data
]]--
function _M.recv_packet(self)
    local sync_bytes, full_len, pos, bodylen, typ, body, binarydata

    local sock = self.sock
    local data, err = sock:receive(4)   -- packet header
    if not data then
        return nil, nil, nil, nil, "failed to receive packet header: " .. err
    end
    --  Sync Bytes 
    sync_bytes = _get_byte4(data, 1)

    -- Full Message Length
    data, err = sock:receive(4)
    full_len = _get_byte4(data, 1)

    if full_len <= TOTAL_HEAD_LEN then
        return nil, nil, nil, nil, "invalid head len"
    end

    if full_len > MAX_MESSAGE_SIZE then
        return nil, nil, nil, nil,  "message size too big, len: " .. full_len
    end

    data, err = sock:receive(full_len - 8)  -- full_len-8 is the message len
    if not data then
        return nil, nil, nil, nil,  "failed to receive the rest message conent, err: " .. err
    end

    pos = 1
    typ, pos = _get_byte4(data, pos)        -- Message Type
    bodylen, pos = _get_byte4(data, pos)   -- body Length

    local body_end = pos + bodylen
    body = string.sub(data, pos, body_end - 1)          -- body

    if full_len - bodylen - TOTAL_HEAD_LEN > 0 then
        binarydata = string.sub(data, body_end)         -- binarydata 
    end

    return  sync_bytes, typ, body, binarydata
end

--[[
openresty业务中存在同时发送情况，因此需要加锁(resty.lock)
而测试用例(test)也需要send，因此增加send_packet2函数
]]--
function _M.send_packet2(self, sync_bytes, typ, body)
    
    local  sock = self.sock
    local message, full_len
    local bodylen = #body

    message =  _set_byte4(typ) .. _set_byte4(bodylen) .. body 
    full_len = bodylen + TOTAL_HEAD_LEN

    message = _set_byte4(sync_bytes) .. _set_byte4(full_len) .. message

    return sock:send(message)
end

 
function _M.new(self, sock)
    if not sock then
        return nil, 'sock is nil'
    end
    return setmetatable({sock = sock, packet_no = 0}, mt)
end

return _M
