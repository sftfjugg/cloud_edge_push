--[[
程序有两个协程：主协程和子协程；
主协程中循环读取tcp缓冲区数据并分发到message_proc中处理
子协程维护一个循环，订阅reids并将收到的数据发送给节点客户端
]] --
local cjson = require("cjson.safe")
local util = require("utils.utility")
local packet = require("stream.message_packet")
local msg_proc = require("stream.message_proc")
local msg_queue = ngx.shared.msg_queue
local _M = { _VERSION = "0.1.0" }

function _M.new(self, sock)
    local err
    if not sock then
        sock, err = ngx.req.socket()
        if not sock then
            ngx.log(ngx.ERR, "unable to obtain request socket: ", err)
            return
        end
    end
    return setmetatable({ sock = sock, client = packet:new(sock) }, { __index = _M })
end

local function _session_exit(session)
    session.exit = true

    -- session退出后，清空队列所占的共享内存空间
    msg_queue:delete(session.sessionid)
    msg_queue:flush_expired()
end

local function _push_msg_to_sendqueue(session, typ, body)
    local body = body
    if type(body) == "table" then
        cjson.encode_empty_table_as_object(false)
        body = cjson.encode(body)
    end
    local msg = packet.packet_message(session.sync_bytes, typ, body)

    local rec, err = msg_queue:rpush(session.sessionid, msg) -- 添加到每个session的消息队列中
    if not rec then
        ngx.log(ngx.ERR, 'push msg to queue fail!: ', err)
    end
end

local function _main_proc(session)
    print('_main_proc... ')
    session.no_heart_time = 0 -- current total number of heartbeat was not received.
    session.heart_beat_flag = false
    while session and session.exit ~= true do
        local sync_bytes, typ, body, binarydata, err = session.client:recv_packet()
        if not typ or not body or err then
            ngx.log(ngx.ERR, 'recv_packet error: ', err)

            msg_proc.session_broken_clean(session)
            _session_exit(session)
            print('===========================_main_proc exit! ===========================')
            return
        end
        session.sync_bytes = sync_bytes
        ngx.log(ngx.DEBUG, '[recv_packet] ', 'sync_bytes: ', sync_bytes, ', typ: ', typ, ', body: ', body)

        -- message handler
        typ, body, err = msg_proc.proc(session, typ, body, binarydata)
        if typ and body and not err then
            session.push_msg_to_sendqueue(session, typ, body)
        elseif err then
            ngx.log(ngx.ERR, 'err: ' .. err)
            _session_exit(session)
            return
        end
    end
end


-- send message to tcpclient
local function _send_message(session)
    local sock = session.client.sock
    while session and session.exit ~= true do
        local message, err = msg_queue:lpop(session.sessionid)
        if not message then
            ngx.sleep(0.002) -- 2ms
        else
            ngx.log(ngx.DEBUG, 'message: ', util.dump(message))
            local bytes, err = sock:send(message)
            if err then
                ngx.log(ngx.ERR, 'send_packet error: ', err)
                _session_exit(session)
                return
            end
        end
    end
    print('===========================_send_message exit! ===========================')
end

function _M.run(session)
    print("stream server run............................................\n")

    session.ip = ngx.var.remote_addr -- client ip address
    session.port = ngx.var.remote_port -- client ip port
    session.sessionid = util.sessionid(session.ip, session.port) -- example: client_127_0_0_1_1235

    session.push_msg_to_sendqueue = _push_msg_to_sendqueue
    local send_thread = ngx.thread.spawn(_send_message, session) -- subscribe the msg_queue then send message to client
    _main_proc(session)
    ngx.sleep(0.1)

    local ok, wait_res = ngx.thread.wait(send_thread) -- waiting for all threads
    print("ok: ", ok, ", wait_res: ", wait_res)

    session = nil
    collectgarbage("collect") -- call GC to reclaim memory
    print("stream server exit............................................\n")

end

return _M
