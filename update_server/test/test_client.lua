package.path = package.path ..';../app/stream/?.lua;../app/?.lua;/usr/local/openresty/lualib/?.lua;/usr/local/openresty/lualib/?.lua;/usr/share/lua/5.1/?.lua;;';
package.cpath = package.cpath ..'/usr/local/openresty/lualib/?.so;;' 

local socket = require("socket")
local packet = require("message_packet")

---------------------for io.read(1)---------------------------
--http://cn.voidcc.com/question/p-ymmsfyxd-ck.html--
local p = require("posix") 
local bit32 = require("bit32") 
local function table_copy(t) 
    local copy = {} 
    for k,v in pairs(t) do 
    if type(v) == "table" then 
     copy[ k ] = table_copy(v) 
    else 
     copy[ k ] = v 
    end 
    end 
    return copy 
end 

assert(p.isatty(p.STDIN_FILENO), "stdin not a terminal") 

-- derive modified terminal settings from current settings 
local saved_tcattr = assert(p.tcgetattr(p.STDIN_FILENO)) 
local raw_tcattr = table_copy(saved_tcattr) 
raw_tcattr.lflag = bit32.band(raw_tcattr.lflag, bit32.bnot(p.ICANON)) 
raw_tcattr.cc[ p.VMIN ] = 0 
raw_tcattr.cc[ p.VTIME ] = 10 -- in tenth of a second 

-- restore terminal settings in case of unexpected error 
local guard = setmetatable({}, { __gc = function() 
    p.tcsetattr(p.STDIN_FILENO, p.TCSANOW, saved_tcattr) 
end }) 

local function read1sec() 
    assert(p.tcsetattr(p.STDIN_FILENO, p.TCSANOW, raw_tcattr)) 
    local c = io.read(1) 
    assert(p.tcsetattr(p.STDIN_FILENO, p.TCSANOW, saved_tcattr)) 
    return c 
end 
-----------------------------------------------------------

--------消息号---------
local SYNC_BYTES_HREAD = 0x01010101 --消息识别码
local kLoginReq = 2001
local kLoginRsp = 2002
local kHeartReq = 2011
local kHeartRsp = 2012
local kUpdateFileNtf = 2023
---------------------------

local host
local port
if arg then
    host = arg[1] or host
    port = arg[2] or port
end
host = host or "127.0.0.1"
port = port or 9999


local len, typ, input, recvt, sendt, status, syn_byte
local client, sock

local function connect()
    if sock then
        sock:close()
    end
    print("Attempting connection to host '" .. host .. "' and port " .. port .. "...")
    sock = socket.connect(host, port)
    if not sock then
        print('!!!error: connect ' .. host ..": ".. port .. ' failed!!!')
        return 
    end
    client = packet:new(sock)
    if not client then
        print('!!!new packet client failed!!!')
    end
    return true
end


local function sendmessage( input )
    if input == "1" then -- login --
        input = '{"timestamp":"20220606140128","msg_type":"login_req","data":{"ip": "127.0.0.1", "type": "SUZHOU"}}'
        typ = kLoginReq

    elseif input == "2" then -- heart -- 
        input = '{"timestamp":"20220606140128","msg_type":"heart_req","data":{"ip": "127.0.0.1"}}'
        typ = kHeartReq

    else
        print("\ninput " .. input .. ' not yet set!!!')
        return true
    end
    
    if input and #input > 0 then
        syn_byte = SYNC_BYTES_HREAD
        print("[sendmessage] type: " .. typ .. ", message: " .. input)
        local sent = client:send_packet2(syn_byte, typ, input)
        if not sent then
            print('send_packet failed!')
            return false
        end
    end
    return true
end

local produceFunc = function()
    while true do
        print("Press enter after input something:")
        input = read1sec()
        if input and #input > 0 then 
            input = string.gsub(input, "%s", "")
        end
        if input and #input > 0 then
            if not sendmessage(input) then
                break
            end
            print("\n")
        end
        coroutine.yield() 
    end
end

local function receive_message( ... )
    sock:settimeout(1) 
    local sync_bytes, typ, body, binarydata, err = client:recv_packet()
    if err then
        if string.find(err, 'closed') then
            print('tcp session closed, try reconnect...\n')
            if not connect() then
                print('!!!reconnect failed!!!\n')
                return false
            else
                print('reconnect success, re-send latest input: '.. input ..' ...\n')
                if not sendmessage(input) then
                    print('sendmessage faild!==================')
                    return false
                else
                    sock:settimeout(0) 
                    receive_message()
                end
            end
        elseif string.find(err, 'timeout') then
            sock:settimeout(0) 
        end
    else
        print("[response] -->>>>>>")
        print(sync_bytes, typ, body ) 
        print("\n")
    end
    return true
end

local consumer = function(p)
    while true do
        coroutine.resume(p)
        if not receive_message() then
            break
        end
    end
end

if not connect() then
    return
end
local producer = coroutine.create(produceFunc)
consumer(producer)

print("exit...")



