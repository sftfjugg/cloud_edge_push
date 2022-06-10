package.path = package.path ..';../../app/stream/?.lua;../../app/?.lua;/usr/local/openresty/lualib/?.lua;/usr/local/openresty/lualib/?.lua;/usr/share/lua/5.1/?.lua;;';
package.cpath = package.cpath ..'/usr/local/openresty/lualib/?.so;;' 

-- cuclient.lua
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
-----------------------------------------------------------

--------消息好---------
local SYNC_BYTES_HREAD = 0x01010101 --消息识别码
local kLoginReq = 2001
local kLoginRsp = 2002
local kHeartReq = 2011
local kHeartRsp = 2012
local kUpdateFileNtf = 2023
local TEST_PUSH_NTF = 2033
---------------------------

if arg then
    host = arg[1] or host
    port = arg[2] or port
end

local host = host or "127.0.0.1"
local port = port or 9999


local len, typ, input, recvt, sendt, status, syn_byte
local client
local sent_a_message = false

local function connect()
    if sock then
        sock:close()
    end
    print("Attempting connection to host '" .. host .. "' and port " .. port .. "...")
    sock = socket.connect(host, port)
    if not sock then
        print('!!!error: connect ' .. host ..": ".. port .. ' failed！！!')
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
        input = '{"timestamp":"20220606140128","msg_type":"login_req","data":{"ip": "127.0.0.1"}}'
        typ = kLoginReq

    elseif input == "2" then -- heart -- 
        input = '{"timestamp":"20220606140128","msg_type":"heart_req","data":{"process_id": "8888"}}'
        typ = kHeartReq

    elseif input == "3" then -- send device list rsp
        input = '{"timestamp":"20220606140128","msg_type":"devicelist_ntf","data":{"groups":[{"group_name":"test1","group_id":"10","group_desc":""}],"devices":[{"device_status":"2","device_ip":"10.40.39.83","device_name":"","device_type":"1","device_password":"abc12345","connect_info":"http://10.40.39.83/onvif/device_service","device_desc":"","device_id":"4447CC9FA4180AB","device_username":"admin", "main_url" : "rtsp://10.40.29.188:554/4447CC9FA418_0/real?", "sub_url" : "rtsp://10.40.29.188:554/4447CC9FA418_1/real?", "group_id" : "10"}]}}'
        typ = kUpdateFileNtf

    else
        -- print(input .. ' not yet set!!!')
        return false
    end
    

    if input and #input > 0 then
        syn_byte = SYNC_BYTES_HREAD
        print("[sendmessage] type: " .. typ .. ", message: " .. input)
        sent = client:send_packet2(syn_byte, typ, input)
        if not sent then
            print('send_packet failed!')
            return false
        end
    end
    sent_a_message = true
    return true
end

local produceFunc = function()
    while true do
        print("Press enter after input something:")
        input = io.read(1)
        if input then
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
        coroutine.resume(p);      
        if not receive_message() then
            break
        else
            sent_a_message = false  -- reset
        end
    end
end

if not connect() then
    return
end
producer = coroutine.create(produceFunc)
consumer(producer)

print("exit...")



