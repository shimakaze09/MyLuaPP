SET(c_options "")
IF (MSVC)
    LIST(APPEND c_options "/wd5030;/permissive-")
ENDIF ()

ADD_TARGET(
        TEST
        MODE EXE
        LIB
        My::MyLuaPP_core
        C_OPTION
        ${c_options}
)