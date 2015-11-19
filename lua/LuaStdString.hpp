
#if !defined(__LUA_1_STD__STRING__)
#define __LUA_1_STD__STRING__

#include "LuaEngine.hpp"
#include <string>

class LuaStdString : 
    public std::string {
    typedef std::string Super ;
public:

    LuaStdString(const char * d ):Super( d ){}
    using Super::c_str  ;
    using Super::append ;
public:
    
    void static registerLUA(  lua_State * eng );

};

#endif

