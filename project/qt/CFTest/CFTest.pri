# this line makes qmake parse this file ONCE not three times
CONFIG -= debug_and_release
QT -= core gui

# this line makes the compile output just show name of file, not all the cmd line args
CONFIG += SILENT

message(------------------------------)

#for(var, $$list($$enumerate_vars())) {
#    message($$var)
#    message($$eval($$var))
#	message(------)
#}

TEMPLATE = app
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += _QT_

DEFINES += _CFTEST_
DEFINES += _MIN_CF_
DEFINES += _JUST_CFTEST_
CONFIG += c++11

greaterThan(QT_MAJOR_VERSION, 5) {
	DEFINES += _QT6_=1
	_QT6_ = 1

	# later you can do this:
	#	equals(_QT6_, 1) {
	#		do something here
	#	}

	win32 {
		CONFIG += no_utf8_source
		QMAKE_CXXFLAGS += /source-charset:.10000	# macroman code page
	}
} else {
	DEFINES += _QT6_=0
	_QT6_ = 0
}

macx {
	DEFINES += OPT_MACOS=1
	DEFINES += OPT_WINOS=0

	# on mac, you can ONLY build 64bit, there is no 32bit compiler
	QMAKE_TARGET.arch = x86_64

} else {
	DEFINES += OPT_MACOS=0
	DEFINES += OPT_WINOS=1

	# options: openssl | libressl
	ssl_edition = libressl

	CONFIG += no_utf8_source
	QMAKE_CXXFLAGS += /source-charset:.10000
}

contains(QMAKE_TARGET.arch, x86_64) {
	DEFINES += __LP64__=1
	DEFINES += __LLP64__=1
	build_depth = 64
} else {
	build_depth = 32
}

CONFIG(release, debug|release) {
	build_type = release
	build_Type = Release
	BUILD_TYPE_SUFFIX = ""
} else {
	build_type = debug
	build_Type = Debug
	DEFINES += kDEBUG
	BUILD_TYPE_SUFFIX = " Debug"
}

message(build_type: $${build_type})
message(target: $${TARGET} ($${build_depth}bit))

# relative to shadow build dir, not proj dir??
CUR_DIR			= $$clean_path($${PWD})/
DIR_PROJ		= $$clean_path($${CUR_DIR}../../)/
DIR_CFTEST		= $$clean_path($${DIR_PROJ}../)/
DIR_CF			= $$clean_path($${DIR_CFTEST}../CF)/

#message(CUR_DIR: $${CUR_DIR})
#message(DIR_PROJ: $${DIR_PROJ})
#message(DIR_CFTEST: $${DIR_CFTEST})
#message(DIR_CF: $${DIR_CF})

DIR_SOURCE		= $${DIR_CFTEST}source/
DIR_OPENCFL		= $${DIR_CF}opencflite-476.17.2/
DIR_CFNET		= $${DIR_CF}CFNetwork/
DIR_CFNET_INC	= $${DIR_CFNET}include/

INCLUDEPATH		+= $${DIR_SOURCE}headers
STD_AFX			= $${DIR_SOURCE}headers/stdafx.h

HEADERS			+= $${STD_AFX}
PRECOMPILED_HEADER = $${STD_AFX}

macx {
	FRAMEWORKS_DIR	= /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks
	QMAKE_LFLAGS += -F$${FRAMEWORKS_DIR}

	# this  hack somehow lets CFTest on mac get into CFLite/CoreFoundation/CFStream.h
	INCLUDEPATH += $${DIR_OPENCFL}dist

	#INCLUDEPATH += $${FRAMEWORKS_DIR}	//	automatic due to QMAKE_LFLAGS 
	
	LIBS += -framework CoreServices
	LIBS += -framework CoreFoundation
	LIBS += -framework ApplicationServices

	# QMAKE_MAC_SDK = macosx10.13
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7

	QMAKE_CXXFLAGS_WARN_ON += -Wall -Wno-unused-parameter -Wno-invalid-source-encoding

	SOURCES += \
		$${DIR_CFNET}NetServices/CFNetworkAddress.c

	HEADERS += \
		$${DIR_CFNET_INC}CFNetwork/CFNetworkAddress.h

} else {
	DEFINES += __WIN32__
	DEFINES += _WINDOWS
	DEFINES += WIN32
	DEFINES += _WIN32
	# ?? DEFINES += TARGET_RT_LITTLE_ENDIAN=1
	
	# calling convention: __cdecl
	QMAKE_CFLAGS += /Gd
	QMAKE_CXXFLAGS += /Gd
    QMAKE_CXXFLAGS += -MP

	# unreferenced param
	QMAKE_CXXFLAGS_WARN_ON += /wd4100

	DEFINES += _UNICODE
	DEFINES += UNICODE
	DEFINES += _SCL_SECURE_NO_WARNINGS
	DEFINES += _CRT_SECURE_NO_WARNINGS

	CFLITE_LIB_NAME = CFLite$${BUILD_TYPE_SUFFIX}
	
	REL_DIR_CFLITE = CFLite/win_$${BUILD_KIT}/$${CFLITE_LIB_NAME}/

	#message(CFLite Dir: $${DIR_OPENCFL}Qt/$$REL_DIR_CFLITE)
	LIBS += -L$${DIR_OPENCFL}Qt/$${REL_DIR_CFLITE} -l$${CFLITE_LIB_NAME}

	CONFIG(release, debug|release) {
		# mulithread dynamic link
		QMAKE_CXXFLAGS += /MD
		LIBS += -lmsvcrt
	} else {
		# mulithread dynamic link
		QMAKE_CXXFLAGS += /MDd
		LIBS += -lmsvcrtd

		DEFINES += _DEBUG
	}
	
	LIBS += -lOle32 -lUser32
	LIBS += -lWs2_32 -lkernel32 -loleaut32 -lgdi32 -lwinspool -lcomdlg32 -ladvapi32 -lshell32 -luuid -lodbc32 -lodbccp32 -lcomctl32

	DIST_INCLUDE = $${DIR_OPENCFL}dist/include/
	INCLUDEPATH += $${DIST_INCLUDE}
	DEPENDPATH	+= $${DIST_INCLUDE}
}

QMAKE_POST_LINK += python $${DIR_PROJ}post_build_cftest.py QT $${BUILD_KIT} $$TARGET $$ssl_edition

INCLUDEPATH += $${DIR_SOURCE}
INCLUDEPATH += $${DIR_SOURCE}shared
INCLUDEPATH += $${DIR_SOURCE}main
INCLUDEPATH += $${DIR_CFNET_INC}	# needed on windows
INCLUDEPATH += $${DIR_CFNET_INC}CFNetwork

SOURCES += \
	$${DIR_SOURCE}shared/CCFData.cpp \
	$${DIR_SOURCE}shared/CCFError.cpp \
	$${DIR_SOURCE}shared/CFBonjour.cpp \
	$${DIR_SOURCE}shared/CFUtils.cpp \
	$${DIR_SOURCE}shared/CNetHTTP.cpp \
	$${DIR_SOURCE}shared/SuperString.cpp \
	$${DIR_SOURCE}main/CFNetworkTest.cpp \
	$${DIR_SOURCE}main/CFTest.cpp \
	$${DIR_SOURCE}main/CFTestUtils.cpp \
	$${DIR_SOURCE}main/CWebServerTest.cpp \
	$${DIR_SOURCE}main/main.cpp

HEADERS += \
    $${DIR_SOURCE}shared/CCFData.h \
    $${DIR_SOURCE}shared/CCFError.h \
    $${DIR_SOURCE}shared/CFBonjour.h \
    $${DIR_SOURCE}shared/CFMacTypes.h \
    $${DIR_SOURCE}shared/CFUtils.h \
    $${DIR_SOURCE}shared/CNetHTTP.h \
    $${DIR_SOURCE}shared/QDUtils.h \
    $${DIR_SOURCE}shared/SuperString.h \
	$${DIR_SOURCE}main/CFNetworkTest.h \
	$${DIR_SOURCE}main/CFTest.h \
	$${DIR_SOURCE}main/CFTestUtils.h \
	$${DIR_SOURCE}main/CWebServerTest.h
