#include "MainTest.hpp"
#include <QApplication>
#include <lua/LuaEngine.hpp>

MainTest::MainTest()
{

}

#include <iostream>

int xxxtest( lua_State * s ) {
    std::cout<<"xxxtest"<<std::endl;
    
    lua_pushnumber(s,123);
    return 1;
}

class Test {
public:
    Test() { std::cout<<"Test"<<std::endl; }
    ~Test() { std::cout<<"~Test"<<std::endl; }
};

int Test_Construct( lua_State *L ) {

    std::shared_ptr<Test> test(new Test);
    void * userData = lua_newuserdata( L , sizeof( std::shared_ptr<Test>  ) );
    new(userData) std::shared_ptr<Test>( test );

    /* 设置元表 */
    luaL_getmetatable(L, "@@Test");  
    lua_setmetatable(L,-2);  

    return 1;
}

template<typename T>
void destory( T * d ) { 
    d->~T();
}

int Test_Destruct(lua_State *L) {
    destory((std::shared_ptr<Test> *)lua_topointer(L,-1))  ;
    return 0;
}

int Test_Print(lua_State *L) {
    std::cout<<"test print"<<std::endl;
    return 0;
}
 
template<typename T_>class FunctionPointer {
public:
    const char * functionName;
    T_ functionPointer;
    typedef T_ type;
    FunctionPointer(  const char * fn , T_ fp):
        functionName(fn),
        functionPointer(fp) {}
};

int main(int argc,char ** argv)  { 
      

QApplication app( argc,argv );

    LuaEngine engine ;
    std::cout<<engine.getTop()<<std::endl;
    engine.eval("print(12+mathsin(12))" );
    engine.eval("abc=12");
    engine.setGlobalNumber("fabc",22);
    std::cout<< engine.getGlobalNumber("fabc") <<std::endl ;
    
    engine.setGlobalString("fstr","string");
    std::cout<<engine.getGlobalString("fstr")<<std::endl;

    engine.setGlobalCFunction("xtest",&xxxtest);
    engine.eval("print( xtest() )" );
    std::cout << engine.getGlobalCFunction("xtest") <<std::endl ;

    auto * L=engine.getLuaState();
    std::cout << luaL_newmetatable(L,"@@Test") <<std::endl;  

    //__gc是lua默认的清理函数  
    lua_pushstring(L,"__gc");  
    lua_pushcfunction(L,&Test_Destruct);  
    lua_settable(L,-3);//公共函数调用的实现就在此啊  

    lua_pushstring(L,"xprint");  
    lua_pushcfunction(L,&Test_Print);  
    lua_settable(L,-3);//公共函数调用的实现就在此啊  

    lua_pushstring(L,"__index");  
    lua_pushvalue(L,-2);  
    lua_settable(L,-3);  
       
    lua_pop(L,1);  

    engine.setGlobalCFunction("Test",&Test_Construct);

    engine.eval( " function xxyy() local tt = Test(); tt:xprint();  end  " );
    engine.eval(" yyyd = StdString(\"meta call xxxx\") ; print(  StdString(\"123\") .. yyyd .. \"333\"  ) ");
    engine.eval( " xxyy() " );

    std::cout<<*engine.getGlobalStdString("yyyd")<<std::endl;
    
  //  lua_gc(L, LUA_GCCOLLECT, LUA_GCSETSTEPMUL);

    std::cout<<engine.getTop()<<std::endl;
    MainTest test;
    test.show();

    std::cout<<LuaEngine::Register<int>::index()<<std::endl;
    std::cout<<LuaEngine::Register<int>::index()<<std::endl;
    std::cout<<LuaEngine::Register<double>::index()<<std::endl;
    std::cout<<LuaEngine::Register<LuaStdString>::index()<<std::endl;
    
return app.exec();

#ifdef _MSC_VER
system("pause");
#endif // _MSC_VER

}


//type meta_table_name 
