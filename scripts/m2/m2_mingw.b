; m2 Mingw-w64 template


system-qt: true

win32: func [blk] [do blk]
msvc: false

console: does [ct/cfg_console: true]

generate_makefile: does [
    foreach t targets [
        ;probe t

        ; make_obj_rule must be called before we write DIST_FILES since
        ; it finds the header dependencies.
        t/built_obj_rule: t/make_obj_rule
    ]

    emit do_tags copy gnu_header
    emit case [
        not system-qt
            {MOC      = $(QTDIR)/bin/moc^/QTINC    = $(QTDIR)/include^/^/}
        eq? qt-version 5
            {MOC      = /usr/x86_64-w64-mingw32/bin/qt5/moc^/QTINC    = /usr/x86_64-w64-mingw32/sys-root/mingw/include/qt5^/^/}
        true             {MOC      = moc-qt4^/QTINC    = /usr/include^/^/}
    ]

    emit "^/#------ Target settings"
    foreach t targets [emit "^/^/" t/macro_text]

    emit [{^/ARCHIVE = } project_version {^/DIST_FILES = \^/}]
    ifn empty? distribution_files [
        emit expand_list_gnu/part distribution_files
    ]
    emit expand_list_gnu header_files_used

    emit "^/^/#------ Build rules^/^/all:"
    emit-sub-project-targets
    foreach t targets [emit [' ' t/output_file]]
    emit eol

    emit-sub-projects
    foreach t targets [emit ' ' t/rule_text]

    emit ["^/^/" do_tags copy gnu_other_rules]

    emit-sub-project-clean
    foreach t targets [emit t/clean]

    emit "^/^/#------ Compile rules^/^/"
    foreach t targets [
        emit t/built_obj_rule
        if t/cfg/qt [emit t/moc_rule]
    ]

    emit "^/#EOF^/"
]


qt-includes: context [
    concurrent: " $(QTINC)/QtConcurrent"
    core:       " $(QTINC)/QtCore"
    gui:        " $(QTINC)/QtGui"
    network:    " $(QTINC)/QtNetwork"
    opengl:     " $(QTINC)/QtOpenGL"
    printsupport: " $(QTINC)/QtPrintSupport"
    svg:        " $(QTINC)/QtSvg"
    sql:        " $(QTINC)/QtSql"
    support: does [
        cxxflags "-DQT3_SUPPORT"
        " $(QTINC)/Qt3Support"
    ]
    widgets: does [
        if lt? qt-version 5 [return ""]
        " $(QTINC)/QtWidgets"
    ]
    xml:        " $(QTINC)/QtXml"
]

qt-libraries: func [debug | blk it] [
    blk: select [
        5 [
            concurrent: " Qt5Concurrent"
            core:    " Qt5Core"
            gui:     " Qt5Gui"
            network: " Qt5Network"
            opengl:  " Qt5OpenGL"
            printsupport: " Qt5PrintSupport"
            svg:     " Qt5Svg"
            sql:     " Qt5Sql"
            widgets: " Qt5Widgets"
            xml:     " Qt5Xml"
        ]
        4 [
            core:    " QtCore"
            gui:     " QtGui"
            network: " QtNetwork"
            opengl:  " QtOpenGL"
            svg:     " QtSvg"
            sql:     " QtSql"
            support: " Qt3Support"
            xml:     " QtXml"
        ]
    ] qt-version

    if debug [
        map it blk [
            either string? it [join it "_debug"][it]
        ]
    ]
    context blk
]


exe_target: make target_env
[
    ; built_obj_rule exists only to hold the output make_obj_rule
    built_obj_rule: none

    obj_macro: none
    cfg_console: false

    append defines "_WIN32"

    config:
    [
        obj_macro: rejoin [ "$(" uc_name "_OBJECTS)" ]

        cflags {-pipe}

        ifn cfg_console [
            lflags {-mwindows}
        ]

        if cfg/warn [
            cflags {-Wall -W}
        ]

        if cfg/debug [
            cflags {-g -DDEBUG}
        ]

        if cfg/release [
            cflags {-O3 -DNDEBUG}
        ]

        if cfg/opengl [
            libs {opengl32 glu32}
        ]

        if cfg/qt [
            include_from {$(QTINC)}
            include_from rejoin bind copy cfg/qt qt-includes

            if cfg/release [
                cxxflags {-DQT_NO_DEBUG}
            ]
        ]

        if cfg/x11 [
            include_from {/usr/X11R6/include}
            libs_from "/usr/X11R6/lib" {Xext X11 m}
        ]
    ]

    configure: func [| qt-libs] [
        output_file: rejoin [output_dir name ".exe"]
        do config
        if cfg/qt [
            qt-libs: rejoin bind copy cfg/qt qt-libraries cfg/debug
            either system-qt [
                libs qt-libs
            ][
                libs_from "$(QTDIR)/lib" qt-libs
            ]
        ]
    ]

    macro_text: does [
        ifn empty? menv_aflags [
            emit [uc_name "_AFLAGS   = " menv_aflags eol]
        ]

        emit [
            uc_name "_CFLAGS   = " menv_cflags eol
            uc_name "_CXXFLAGS = $(" uc_name "_CFLAGS) " menv_cxxflags eol
            uc_name "_INCPATH  = " gnu_string "-I" include_paths eol
            uc_name "_LFLAGS   = " menv_lflags eol
            uc_name "_LIBS     = " gnu_string "-L" link_paths ' '
                                   gnu_string "-l" link_libs eol
        ]

        emit [
            uc_name "_SOURCES  = " expand_list_gnu source_files eol
            uc_name "_OBJECTS  = " expand_list_gnu object_files eol
        ]
    ]

    clean: does [
        rejoin [
            "^--rm -f " output_file ' ' obj_macro
            either empty? srcmoc_files [""] [join ' ' to-text srcmoc_files]
            eol
        ]
    ]

    dist: does [
        rejoin [" $(" uc_name "_SOURCES)"]
    ]

    rule_text: does
    [
        emit [ eol output_file ": " obj_macro local_libs link_libs
               sub-project-libs link_libs
            {^/^-$(}
                either link_cxx ["LINK_CXX"]["LINK"]
                {) -o $@ $(} uc_name {_LFLAGS) } obj_macro
            { $(} uc_name {_LIBS)} eol
        ]
    ]
]


lib_target: make exe_target [
    configure: does [
        output_file: rejoin [output_dir name ".a"]
        do config
    ]

    rule_text: does
    [
        emit [eol output_file ": " obj_macro sub-project-libs link_libs]
        emit either empty? link_libs [[
            "^/^-ar rc $@ " obj_macro " $(" uc_name "_LFLAGS)"
        ]] [[
            ; Concatenate other libraries.
            "^/^-ld -Ur -o " objdir name "lib.o $^^ $(" uc_name
                "_LIBS) $(" uc_name "_LFLAGS)"
            "^/^-ar rc $@ " objdir name "lib.o"
        ]]
        emit "^/^-ranlib $@^/"
        if cfg/release [
            emit "^-strip -d $@^/"
        ]
    ]
]


shlib_target: make exe_target [
    configure: does [
        lib_full: rejoin [name ".dll"]
        output_file: join output_dir lib_full
        do config
        lflags rejoin ["-shared -Wl,--out-implib," name ".a"]
    ]
]


gnu_header:
{#----------------------------------------------------------------------------
# Makefile for GNU make
# Generated by m2 at <now/date>
# Project: <m2/project>
#----------------------------------------------------------------------------


#------ Compiler and tools

AS       = x86_64-w64-mingw32-as
CC       = x86_64-w64-mingw32-gcc -Wdeclaration-after-statement
CXX      = x86_64-w64-mingw32-g++
LINK     = x86_64-w64-mingw32-gcc
LINK_CXX = x86_64-w64-mingw32-g++
TAR      = tar -cf
GZIP     = gzip -9f

}
;#------ Project-wide settings


gnu_other_rules:
{<m2/makefile>: <project_file>
^-m2 <project_file>

.PHONY: dist
dist:
^-$(TAR) $(ARCHIVE).tar --exclude CVS --exclude .svn --exclude *.o <project_file> <m2/dist_files> $(DIST_FILES)
^-mkdir /tmp/$(ARCHIVE)
^-tar -C /tmp/$(ARCHIVE) -xf $(ARCHIVE).tar
^-tar -C /tmp -cf $(ARCHIVE).tar $(ARCHIVE)
^-rm -rf /tmp/$(ARCHIVE)
^-$(GZIP) $(ARCHIVE).tar

.PHONY: clean
clean:
^--rm -f core
}


;EOF