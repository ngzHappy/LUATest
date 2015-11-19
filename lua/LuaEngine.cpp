﻿#include "LuaEngine.hpp"
#include "LuaStdString.hpp"
#include <thread>
#include <iostream>
#include <utility>
#include <algorithm>
#include <cstring>

/*
static int foo (lua_State *L) {
int n = lua_gettop(L);      number of arguments
lua_Number sum = 0.0;
int i;
for (i = 1; i <= n; i++) {
    if (!lua_isnumber(L, i)) {
        lua_pushliteral(L, "incorrect argument");
        lua_error(L);
    }
    sum += lua_tonumber(L, i);
}
lua_pushnumber(L, sum/n);     //    first result
lua_pushnumber(L, sum);       //    second result
return 2;                     // number of results
     }
*/

/*
lua state
 9 -1
 8 -2
 7 -3
 6 -4
 5 -5
 4 -6
 3 -7
 2 -8
 1 -9
*/

namespace {
    namespace cpplua {
/*copy form lua.cpp*/

        constexpr static const char *progname="lua core";
        /*
        ** Prints an error message, adding the program name in front of it
        ** (if present)
        */
        static void l_message(const char *pname, const char *msg) {
            if (msg==nullptr) { return; }
            if (pname) { std::cout<<pname<<" : "; }
            std::cout<<msg<<std::endl;
            std::cout.flush();
        }

        /*
        ** Check whether 'status' is not OK and, if so, prints the error
        ** message on the top of the stack. It assumes that the error object
        ** is a string, as it was either generated by Lua or by 'msghandler'.
        */
        static int report(lua_State *L, int status) {
            if (status!=LUA_OK) {
                const char *msg=lua_tostring(L, -1);
                l_message(progname, msg);
                lua_pop(L, 1);  /* remove message */
            }
            return status;
        }


        /*
        ** Message handler used to run all chunks
        */
        static int msghandler(lua_State *L) {
            const char *msg=lua_tostring(L, 1);
            if (msg==NULL) {  /* is error object not a string? */
                if (luaL_callmeta(L, 1, "__tostring")&&  /* does it have a metamethod */
                    lua_type(L, -1)==LUA_TSTRING)  /* that produces a string? */
                {
                    std::cout<<msg<<std::endl;
                    return 1;  /* that is the message */
                }
                else
                    msg=lua_pushfstring(L, "(error object is a %s value)",
                    luaL_typename(L, 1));
            }
            //luaL_traceback(L, L, msg, 1);  /* append a standard traceback */
            std::cout<<msg<<std::endl;
            return 1;  /* return the traceback */
        }

    }//~luacpp
}//~

LuaEngineData::LuaEngineData() {
    state=luaL_newstate();
    /* 注册lua默认库 */
    luaL_openlibs(state);  /* open standard libraries */

}

LuaEngineData::~LuaEngineData() {
/*
 * 主线程不关闭
*/
    if(LuaEngine::mainThreadID() == std::this_thread::get_id() ){
        return ;
    }
    lua_close( state );

}

void LuaEngine::setGlobalNumber(const char * value_name, double v) {
    if (value_name==nullptr) { return; }
    auto * L=ldata->state;
    auto top_=lua_gettop(L); /*获得堆栈顶*/
    lua_pushnumber(L, v);
    lua_setglobal(L, value_name);
    lua_settop(L, top_);
}

lua_CFunction LuaEngine::getGlobalCFunction(const char * n) const {
    if (n==nullptr) { return nullptr; }
    auto * L=ldata->state;
    auto top_=lua_gettop(L); /*获得堆栈顶*/
    if (lua_getglobal(L, n)==LUA_TFUNCTION) {
        auto ans=lua_tocfunction(L, -1);
        lua_settop(L, top_);
        return ans;
    }
    lua_settop(L, top_);
    return nullptr;
}

void LuaEngine::setGlobalCFunction(const char * n, lua_CFunction function) {
    if (n==nullptr) { return; }
    if (function==nullptr) { return; }
    auto * L=ldata->state;
    auto top_=lua_gettop(L); /*获得堆栈顶*/
    lua_pushcfunction(L, function);
    lua_setglobal(L, n);
    lua_settop(L, top_);
}

void LuaEngine::setGlobalString(const char * vn, const char * str) {
    if (vn==nullptr) { return; }
    auto * L=ldata->state;
    auto top_=lua_gettop(L); /*获得堆栈顶*/
    lua_pushfstring(L, str?str:"");
    lua_setglobal(L, vn);
    lua_settop(L, top_);

}
std::string LuaEngine::getGlobalString(const char * vn) const {
    if (vn==nullptr) { return std::string(); }
    auto * L=ldata->state;
    auto top_=lua_gettop(L); /*获得堆栈顶*/
    if (lua_getglobal(L, vn)==LUA_TSTRING) {
        std::string ans(lua_tostring(L, -1));
        lua_settop(L, top_);
        return std::move(ans);
    }
    lua_settop(L, top_);
    return std::string();
}

double LuaEngine::getGlobalNumber(const char * value_name) const/* if error it will ruturn  */ {
    if (value_name==nullptr) { return std::numeric_limits< double >::quiet_NaN(); }
    auto * L=ldata->state;
    auto top_=lua_gettop(L); /*获得堆栈顶*/
    if (lua_getglobal(L, value_name)==LUA_TNUMBER) {
        double ans=lua_tonumber(L, -1);
        lua_settop(L, top_); /*还原堆栈顶*/
        return ans;
    }
    else {
        lua_settop(L, top_);
    }
    return std::numeric_limits< double >::quiet_NaN();
}

#ifdef _MSC_VER
#define ThreadLocalMoc thread_local
#endif

std::shared_ptr<LuaEngineData> \
LuaEngineData::instanceData() {

#if defined(ThreadLocalMoc)

    ThreadLocalMoc static std::unique_ptr<
        std::shared_ptr<LuaEngineData>
    > ans(new std::shared_ptr<LuaEngineData>(
        []()->auto{
        auto * L=new LuaEngineData;
        std::shared_ptr<LuaEngineData> a(L,
            [](auto * d) {
            delete d;
        }
        );
        L->threadID=std::this_thread::get_id();
        LuaStdString::registerLUA(L->state);
        return a;
    }()
        ));
    return *ans;

#else

    static thread_local std::unique_ptr<LuaEngineData> data=[]() {
        auto * L=new LuaEngineData;
        L->threadID=std::this_thread::get_id();
        LuaStdString::registerLUA(L->state);
        return std::unique_ptr<LuaEngineData>( L );
    }();

    std::shared_ptr<LuaEngineData> ans(
        data.get(),
        [](auto * ) { }
        );

    return ans;

#endif

}

LuaEngine::LuaEngine()
    :ldata(LuaEngine::instanceData()) {
    mainThreadID();
}

LuaEngine::Status LuaEngine::eval(const char * data, std::size_t length) {
    auto * L=ldata->state;

    if (length==((std::numeric_limits<std::size_t>::max)())) {
        length=std::strlen(data); /* 获得字符串长度 */
    }

    auto status=luaL_loadbuffer(L, data, length, "=(command line)"); /* 解析字符串 */
    if (status!=LUA_OK) {
        cpplua::report(L, status);
        return LuaEngine::Status(status);
    }

    auto base=lua_gettop(L); /* get top of the state */
    lua_pushcfunction(L, cpplua::msghandler);  /* push message handler */
    lua_insert(L, base);  /* put it under function and args */
    status=lua_pcall(L, 0, 0, base);

    if (status!=LUA_OK) {
        setTop(base-1); /* 还原栈区 */
    }
    else {
        lua_remove(L, base);  /* remove message handler from the stack */
    }

    return LuaEngine::Status(status);
}

LuaEngine::~LuaEngine() {
/* 析构lua */

}

namespace {

    namespace _r_std__string__lua__ {

        /* 析构函数 */
        int destruct(lua_State * L) {
            LuaEngine::destory((LuaPointer<LuaStdString> *)(lua_topointer(L, 1)));
            return 0;
        }

        /*构造函数*/
        int construct(lua_State *L) {

            LuaPointer<LuaStdString> test;

            if (lua_isstring(L, 1)) {
                test=LuaPointer<LuaStdString>(new LuaStdString(lua_tostring(L, 1)));
            }
            else {
                test=LuaPointer<LuaStdString>(new LuaStdString(""));
            }

            void * userData=lua_newuserdata(L, sizeof(LuaPointer<LuaStdString>));
            test.typeID=(decltype(test.typeID))(LuaEngine::Register<LuaStdString>::index());
            new(userData) LuaPointer<LuaStdString>(test);

            /* 设置元表 */
            luaL_getmetatable(L, "@@std_string_lua");
            lua_setmetatable(L, -2);

            return 1;
        }

        /*c_str*/
        int c_str(lua_State *L) {
            auto * d=((LuaPointer<LuaStdString> *)lua_topointer(L, 1));
            lua_pushstring(L, d->get()->c_str());
            return 1;
        }

        /*append*/
        int append(lua_State *L) {

            if (lua_isuserdata(L, 1)==false) {
                return 0;
            }

            LuaStdString ans("");

            {
                size_t len;
                const char * l=luaL_tolstring(L, 1, &len);
                const char * r=luaL_tolstring(L, 2, &len);
                ans.append(l?l:"");
                ans.append(r?r:"");
            }

            void * userData=lua_newuserdata(L, sizeof(LuaPointer<LuaStdString>));
            LuaPointer<LuaStdString> __(new LuaStdString(std::move(ans)));
            __.typeID=(decltype(__.typeID))(LuaEngine::Register<LuaStdString>::index());
            new(userData) LuaPointer<LuaStdString>(__);

            /* 设置元表 */
            luaL_getmetatable(L, "@@std_string_lua");
            lua_setmetatable(L, -2);

            return 1;
        }

        /*
        "add": the + operation. If any operand for an addition is not a number (nor a string coercible to a number), Lua will try to call a metamethod. First, Lua will check the first operand (even if it is valid). If that operand does not define a metamethod for the "__add" event, then Lua will check the second operand. If Lua can find a metamethod, it calls the metamethod with the two operands as arguments, and the result of the call (adjusted to one value) is the result of the operation. Otherwise, it raises an error.
        "sub": the - operation. Behavior similar to the "add" operation.
        "mul": the * operation. Behavior similar to the "add" operation.
        "div": the / operation. Behavior similar to the "add" operation.
        "mod": the % operation. Behavior similar to the "add" operation.
        "pow": the ^ (exponentiation) operation. Behavior similar to the "add" operation.
        "unm": the - (unary minus) operation. Behavior similar to the "add" operation.
        "idiv": the // (floor division) operation. Behavior similar to the "add" operation.
        "band": the & (bitwise and) operation. Behavior similar to the "add" operation, except that Lua will try a metamethod if any operand is neither an integer nor a value coercible to an integer (see §3.4.3).
        "bor": the | (bitwise or) operation. Behavior similar to the "band" operation.
        "bxor": the ~ (bitwise exclusive or) operation. Behavior similar to the "band" operation.
        "bnot": the ~ (bitwise unary not) operation. Behavior similar to the "band" operation.
        "shl": the << (bitwise left shift) operation. Behavior similar to the "band" operation.
        "shr": the >> (bitwise right shift) operation. Behavior similar to the "band" operation.
        "concat": the .. (concatenation) operation. Behavior similar to the "add" operation, except that Lua will try a metamethod if any operand is neither a string nor a number (which is always coercible to a string).
        "len": the # (length) operation. If the object is not a string, Lua will try its metamethod. If there is a metamethod, Lua calls it with the object as argument, and the result of the call (always adjusted to one value) is the result of the operation. If there is no metamethod but the object is a table, then Lua uses the table length operation (see §3.4.7). Otherwise, Lua raises an error.
        "eq": the == (equal) operation. Behavior similar to the "add" operation, except that Lua will try a metamethod only when the values being compared are either both tables or both full userdata and they are not primitively equal. The result of the call is always converted to a boolean.
        "lt": the < (less than) operation. Behavior similar to the "add" operation, except that Lua will try a metamethod only when the values being compared are neither both numbers nor both strings. The result of the call is always converted to a boolean.
        "le": the <= (less equal) operation. Unlike other operations, The less-equal operation can use two different events. First, Lua looks for the "__le" metamethod in both operands, like in the "lt" operation. If it cannot find such a metamethod, then it will try the "__lt" event, assuming that a <= b is equivalent to not (b < a). As with the other comparison operators, the result is always a boolean. (This use of the "__lt" event can be removed in future versions; it is also slower than a real "__le" metamethod.)
        "index": The indexing access table[key]. This event happens when table is not a table or when key is not present in table. The metamethod is looked up in table.
        Despite the name, the metamethod for this event can be either a function or a table. If it is a function, it is called with table and key as arguments. If it is a table, the final result is the result of indexing this table with key. (This indexing is regular, not raw, and therefore can trigger another metamethod.)

        "newindex": The indexing assignment table[key] = value. Like the index event, this event happens when table is not a table or when key is not present in table. The metamethod is looked up in table.
        Like with indexing, the metamethod for this event can be either a function or a table. If it is a function, it is called with table, key, and value as arguments. If it is a table, Lua does an indexing assignment to this table with the same key and value. (This assignment is regular, not raw, and therefore can trigger another metamethod.)

        Whenever there is a "newindex" metamethod, Lua does not perform the primitive assignment. (If necessary, the metamethod itself can call rawset to do the assignment.)

        "call": The call operation func(args). This event happens when Lua tries to call a non-function value (that is, func is not a function). The metamethod is looked up in func. If present, the metamethod is called with func as its first argument, followed by the arguments of the original call (args).
        It is a good practice to add all needed metamethods to a table before setting it as a metatable of some object. In particular, the "__gc" metamethod works only when this order is followed (see §2.5.1).
        */

        /* 设置元表 */
        void init_meta_table(lua_State * L) {

            luaL_newmetatable(L, "@@std_string_lua");

            //__gc是lua默认的清理函数
            lua_pushstring(L, "__gc");
            lua_pushcfunction(L, &_r_std__string__lua__::destruct);
            lua_settable(L, -3);

            //__index寻找关键词
            lua_pushstring(L, "__index");
            lua_pushvalue(L, -2);
            lua_settable(L, -3);

            //c_str转为字符串
            lua_pushstring(L, "c_str");
            lua_pushcfunction(L, &_r_std__string__lua__::c_str);
            lua_settable(L, -3);

            //__tostring
            lua_pushstring(L, "__tostring");
            lua_pushcfunction(L, &_r_std__string__lua__::c_str);
            lua_settable(L, -3);

            //__add
            lua_pushstring(L, "__add");
            lua_pushcfunction(L, &_r_std__string__lua__::append);
            lua_settable(L, -3);

            //__concat
            lua_pushstring(L, "__concat");
            lua_pushcfunction(L, &_r_std__string__lua__::append);
            lua_settable(L, -3);

        }

        void init_construct(lua_State * L) {
            lua_pushcfunction(L, &construct);
            lua_setglobal(L, "StdString");
        }

    }

}
/* demo for register std::string */
void LuaStdString::registerLUA(lua_State * eng) {
    using namespace _r_std__string__lua__;

    auto * L=eng;
    auto top_=lua_gettop(L);

    //初始化元表
    init_meta_table(L);
    //初始化构造函数
    init_construct(L);

    lua_settop(L, top_);
}

LuaPointer< LuaStdString >
LuaEngine::getGlobalStdString(const char * n) const {
    if (n==nullptr) { return LuaPointer< LuaStdString >(); }
    auto * L=ldata->state;
    auto top_=lua_gettop(L); /*获得堆栈顶*/
    if (lua_getglobal(L, n)==LUA_TUSERDATA) {
        auto ans=(LuaUserData *)lua_topointer(L, -1);
        if (ans->typeID==LuaEngine::Register<LuaStdString>::index()) {
            lua_settop(L, top_);
            return *((LuaPointer< LuaStdString >*)(ans));
        }

    }
    lua_settop(L, top_);
    return LuaPointer< LuaStdString >();

}



/* endl of the file */


