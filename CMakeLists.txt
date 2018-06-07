cmake_minimum_required(VERSION 3.10)

project(Shell VERSION 0.0.0 DESCRIPTION
        "Another shell"
)

configure_file(manifest.h.in ${PROJECT_SOURCE_DIR}/manifest.h)

add_executable(ISLinux tools.c ish.c parser.h manifest.h)