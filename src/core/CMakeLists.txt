SET(c_options "")
IF (MSVC)
    LIST(APPEND c_options "/wd5030")
ENDIF ()

ADD_TARGET(
        MODE INTERFACE
        INC
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/include/_deps"
        C_OPTION
        ${c_options}
        LIB
        My::MySRefl_core
        My::MyLua_core
)