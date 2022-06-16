local cjson = require("cjson.safe")
local util = require("utils.utility")
-- local repl = require("resty.repl") --断点调试代码库
local msgdef = require("message_def")
local client_types = ngx.shared.tcp_client_types
local notify_msg_queue = ngx.shared.notify_msg_queue

local _M = {
    _VERSION = '0.1.0'
}


function _M.session_broken_clean(session)
    --session退出登录后的处理，清空队列所占的共享内存空间

    session.is_login = false
    notify_msg_queue:delete(session.sessionid)
    notify_msg_queue:flush_expired()

    client_types:delete(session.sessionid)
    client_types:flush_expired()

    if session.notify_msg_thread then
        local ok, wait_res = ngx.thread.wait(session.notify_msg_thread) 
        print("ok: ", ok, ", wait_res: ", wait_res)
        session.notify_msg_thread = nil
    end
end

local function _notify_msg_proc(session)
    while session and session.is_login do
        local message = notify_msg_queue:lpop(session.sessionid)
        if not message then
            ngx.sleep(0.002) -- 2ms
        else
            ngx.log(ngx.DEBUG, 'message: ', message)
            session.push_msg_to_sendqueue(session, msgdef.kUpdateFileNtf, message)
        end
    end
    print('===========================_notify_msg_proc exit! ===========================')
end


local NO_HEART_BEAT_THRESHOLD = 6   -- 连续没收到心跳的上限 
local TIMER_INTERVAL = 10             -- 定时器时间间隔为10s 

local _deal_heart_beat              -- 必须先声明，要不然再次启动定时器会找不到函数
_deal_heart_beat = function(premature, session)
    ngx.log(ngx.DEBUG, "heart*********************beat -- " .. session.no_heart_time)
    if NO_HEART_BEAT_THRESHOLD == session.no_heart_time then
        -- 60s内没有收到一次心跳消息，则认为客户端已经断连 -- 
        _M.session_broken_clean(session)
        ngx.log(ngx.ERR, "no heart beat"..util.dump(session))
        return
    else
        if not session.heart_beat_flag then
            session.no_heart_time = session.no_heart_time + 1   
        else 
            session.no_heart_time = 0   
            session.heart_beat_flag = false
        end
        
        local ok, err = ngx.timer.at(TIMER_INTERVAL, _deal_heart_beat, session)  
        if not ok then
            ngx.log(ngx.ERR, "failed to create timer: ", err)
            return
        end
    end
end


local function _do_login_req(session, typ, tbl)
    local typ, data
    local ret = {}

    typ = msgdef.kLoginRsp
    ret.msg_type = "login_rsp"
    data = { code = 1000 } 

    if not session.is_login then
        session.is_login = true 

        if tbl.data and tbl.data.type then
            session.client_type = tbl.data.type
            client_types:set(session.sessionid, session.client_type)
        end

        session.notify_msg_thread = ngx.thread.spawn(_notify_msg_proc, session)
        
        -- start heart beat timer 10s 
        local ok, err = ngx.timer.at(TIMER_INTERVAL, _deal_heart_beat, session)
        if not ok then
            ngx.log(ngx.ERR, "failed to create timer: ", err)
            return
        end
    end

    return ret, typ, data
end

local function _do_heart_req(session, typ, tbl)
    local typ, data, code
    local ret = {}
    session.heart_beat_flag = true

    if not tbl and not tbl.data then
        code = 1006
        ngx.log(ngx.ERR, 'parameter errors in the heartbeat package')
    else
        code = 1000
    end

    typ = msgdef.kHeartRsp
    ret.msg_type = "heart_rsp"
    data = { code = code }
    return ret, typ, data
end

local function _do_recv_fileinfos_ntf(session, typ, tbl)
    if not tbl and not tbl.data then
        ngx.log(ngx.ERR, 'parameter errors in the send_file_infos package')
    else
    end
    if tbl.data.file_infos then
        print(util.dump(cjson.decode(tbl.data.file_infos)))
    end
end


function _M.proc(session, typ, body, image)
    local data
    local ret = {}
    ngx.log(ngx.INFO, 'recv msg body: '.. body)
    local tbl = cjson.decode(body)
    if typ == msgdef.kLoginReq then
        ret, typ, data = _do_login_req(session, typ, tbl)
        
    elseif typ == msgdef.kHeartReq then
        ret, typ, data = _do_heart_req(session, typ, tbl)
    
    elseif typ == msgdef.kSendFileInfosNtf then
        ret, typ, data = _do_recv_fileinfos_ntf(session, typ, tbl)
        return

    else
        data = { code = "1201" }
        ngx.log(ngx.ERR, 'message type is UNKNOWN! msg_type: ', typ, ' ,body: ', body)
        return
    end
    
    ret.data = data
    ret.timestamp = tbl.timestamp
    body = cjson.encode(ret)

    return typ, body
end

return _M