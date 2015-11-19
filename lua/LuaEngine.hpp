
#if !defined(LUASTACK_HPP__luaeng)
#define LUASTACK_HPP__luaeng

#include "lua.hpp"
#include <memory>
#include <new>
#include <thread>
#include <ciso646>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <cstdbool>
#include <string>
#include "LuaStdString.hpp"

class LuaEngineData{
public:
    lua_State * state = nullptr ;
    std::thread::id threadID;
    static std::shared_ptr<LuaEngineData> instanceData();
    ~LuaEngineData();

private:
    LuaEngineData();

};

class LuaUserData {
public:
    typedef std::size_t size_t;
    static size_t invalid() { return -1; }
    size_t typeID = invalid();
    bool  isInvalidLuaUserData() const{
        return typeID!=invalid();
    }
    LuaUserData() {}
};

template<typename T>
class LuaPointer :
    public LuaUserData,
    public std::shared_ptr<T>{
public:
    typedef std::shared_ptr<T> Super;
    using Super::Super;
};

/*
 * 每个线程使用独立的LUA堆栈
*/
class LuaEngine
{
    std::shared_ptr<LuaEngineData>  ldata  ;
    static std::shared_ptr<LuaEngineData> instanceData(){
        return LuaEngineData::instanceData();
    }
    static LuaUserData::size_t & getIndexID_() {
        static LuaUserData::size_t typeIndex_ =0;
        return typeIndex_;
    }

public:

    static std::thread::id mainThreadID(){
        static std::thread::id ans_ = std::this_thread::get_id();
        return ans_;
    }

    enum class Status : int {
        /* thread status */
OK         =  LUA_OK		,
YIELD      =  LUA_YIELD	    ,
ERRRUN     =  LUA_ERRRUN	,
ERRSYNTAX  =  LUA_ERRSYNTAX	,
ERRMEM     =  LUA_ERRMEM	,
ERRGCMM    =  LUA_ERRGCMM	,
ERRERR     =  LUA_ERRERR	,
    };

    enum TypeID : LuaUserData::size_t {
        User_Define_ID = 0
    };

    template<typename RT_>
    class Register {
    public:
        static LuaUserData::size_t index() {
            static const LuaUserData::size_t id_=(User_Define_ID+(getIndexID_()++));
            return id_;
        }
    };

    LuaEngine();
    virtual ~LuaEngine();

    lua_State * getLuaState()const{return ldata->state ;}
    std::thread::id getThreadID()const{return ldata->threadID;}

    /* this will do string */
    Status eval( const char * data, std::size_t length = (std::numeric_limits<std::size_t>::max)() );
    template< typename SizeType >
    auto eval(const char * data,const SizeType & length) { return eval(data, static_cast<std::size_t>(length) ); }

    int getTop() const { return lua_gettop( ldata->state ); }
    void setTop(int v) { lua_settop( ldata->state ,v); }

    /* if error it will ruturn  std::numeric_limits<double >::quiet_NaN()*/
    double getGlobalNumber(const char * ) const ;
    void setGlobalNumber(const char *, double );

    void setGlobalString(const char *,const char *)  ;
    std::string getGlobalString(const char *) const ;
    void setGlobalString(const char * n, const std::string & v) {  setGlobalString(n,v.c_str()); }
    void setGlobalString(const char * n, std::string && v) { std::string v_=std::move(v); setGlobalString(n,v_.c_str()); }

    void setGlobalCFunction( const char * , lua_CFunction function );
    lua_CFunction getGlobalCFunction( const char * ) const;

    LuaPointer< LuaStdString > getGlobalStdString(const char * n) const;

public:
    template<typename T>
    static void destory(T * d) { if (d) { d->~T(); } }
};

#endif // LUASTACK_HPP
