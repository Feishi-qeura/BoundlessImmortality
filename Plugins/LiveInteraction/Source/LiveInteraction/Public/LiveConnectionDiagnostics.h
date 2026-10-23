#pragma once
#include <cstdint>
#include <string>
#include <string_view>

namespace liveinteraction {
struct ConnectionDiagnostic { std::string stage="starting",operation="init";std::int64_t code=0; };
inline bool IsConnectionStage(std::string_view value) {
    for(const char* known:{"starting","credential_missing","credential_pipe_failed","host_starting","host_ready","sdk_initializing","waiting_room","room_authenticated","subscribing","waiting_push","connected","disconnected","failed"})if(value==known)return true;
    return false;
}
inline bool IsConnectionOperation(std::string_view value) {
    for(const char* known:{"init","host","protocol","room","connection","round","events","subscribe:live_comment","subscribe:live_like","subscribe:live_gift","subscribe:live_team","subscribe:live_enter","subscribe:live_follow"})if(value==known)return true;
    return false;
}
inline ConnectionDiagnostic SanitizeConnectionDiagnostic(std::string_view stage,std::string_view operation,std::int64_t code,bool authenticated) {
    return {stage=="connected"&&!authenticated?"waiting_push":IsConnectionStage(stage)?std::string(stage):"unknown",IsConnectionOperation(operation)?std::string(operation):"unknown",code};
}
// Only startup/connection status may affect this panel. Backend receipts and
// gameplay command failures have their own UI; none may claim authentication.
inline bool ProjectSdkConnection(ConnectionDiagnostic& out,std::string_view operation,bool success,std::int64_t code,std::string_view state,bool authenticated) {
    if(!IsConnectionOperation(operation))return false;
    if(state=="failed"&&!success){out=SanitizeConnectionDiagnostic("failed",operation,code,authenticated);return true;}
    if(operation=="connection"){
        out=SanitizeConnectionDiagnostic(success&&state=="connected"?(authenticated?"connected":"waiting_push"):"disconnected",operation,code,authenticated);return true;
    }
    if(authenticated)return false;
    std::string_view stage;
    if(operation=="host"&&success&&state=="ready")stage="host_ready";
    else if(operation=="init"){
        stage=state=="waiting_room"?"waiting_room":state=="room_authenticated"?"room_authenticated":state=="all_subscribed"?"waiting_push":"sdk_initializing";
        if(!success)stage="failed";
    }else if(operation.substr(0,10)=="subscribe:")stage=success?"subscribing":"failed";
    else if(operation=="protocol"&&!success)stage="failed";
    else return false;
    out=SanitizeConnectionDiagnostic(stage,operation,code,authenticated);return true;
}
}
