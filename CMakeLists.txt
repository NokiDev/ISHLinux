cmake_minimum_required(VERSION 3.10)

project(YnovShell VERSION 1.0.0
        LANGUAGES C
        DESCRIPTION
            "Another shell"
)

configure_file(manifest.h.in ${PROJECT_SOURCE_DIR}/manifest.h)

add_executable(Ysh
        ish.c
        tools.c tools.h
        parser.h
        manifest.h
        defines.h
        commands.c commands.h
)
