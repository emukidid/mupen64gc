#/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
# *   Mupen64plus - pre.mk                                                  *
# *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
# *   Copyright (C) 2007-2008 DarkJeztr Tillin9 Richard42                   *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU General Public License as published by  *
# *   the Free Software Foundation; either version 2 of the License, or     *
# *   (at your option) any later version.                                   *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU General Public License for more details.                          *
# *                                                                         *
# *   You should have received a copy of the GNU General Public License     *
# *   along with this program; if not, write to the                         *
# *   Free Software Foundation, Inc.,                                       *
# *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

# detect system architecture: i386, x86_64, or PPC/PPC64
UNAME = $(shell uname -m)
ifeq ("$(UNAME)","x86_64")
  CPU = X86
  ifeq ("$(BITS)", "32")
    ARCH = 64BITS_32
  else
    ARCH = 64BITS
  endif
endif
ifneq ("$(filter i%86,$(UNAME))","")
  CPU = X86
  ARCH = 32BITS
endif
ifeq ("$(UNAME)","ppc")
  CPU = PPC
  ARCH = 32BITS
  NO_ASM = 1
endif
ifeq ("$(UNAME)","ppc64")
  CPU = PPC
  ARCH = 64BITS
  NO_ASM = 1
endif

# detect operation system. Currently just linux and OSX.
UNAME = $(shell uname -s)
ifeq ("$(UNAME)","Linux")
  OS = LINUX
endif
ifeq ("$(UNAME)","linux")
  OS = LINUX
endif
ifeq ("$(UNAME)","Darwin")
  OS = OSX
  LDFLAGS += -liconv -lpng
endif

ifeq ($(OS),)
   $(warning OS not supported or detected, using default linux options.)
   OS = LINUX
endif

# test for presence of SDL
ifeq ($(shell which sdl-config 2>/dev/null),)
  $(error No SDL development libraries found!)
endif
SDL_FLAGS	= $(shell sdl-config --cflags)
SDL_LIBS	= $(shell sdl-config --libs)

# test for presence of FreeType
ifeq ($(shell which freetype-config 2>/dev/null),)
   $(error freetype-config not installed!)
endif
FREETYPE_LIBS	= $(shell freetype-config --libs)
FREETYPE_FLAGS	= $(shell freetype-config --cflags)

# set Freetype flags
FREETYPEINC = $(shell pkg-config --cflags freetype2)
CFLAGS += $(FREETYPEINC)

# detect GUI options
ifeq ($(GUI),)
  GUI = GTK2
endif

# set GTK2 flags and libraries
# ideally we don't always do this, only when using the Gtk GUI,
# but too many plugins require it...

# test for presence of GTK 2.0
ifeq ($(shell which pkg-config 2>/dev/null),)
  $(error pkg-config not installed!)
endif
ifneq ("$(shell pkg-config gtk+-2.0 --modversion | head -c 2)", "2.")
  $(error No GTK 2.x development libraries found!)
endif
# set GTK flags and libraries
GTK_FLAGS	= $(shell pkg-config gtk+-2.0 --cflags)
GTK_LIBS	= $(shell pkg-config gtk+-2.0 --libs)
GTHREAD_LIBS 	= $(shell pkg-config gthread-2.0 --libs)

# set Qt flags and libraries
# some distros append -qt4 to the binaries so look for those first
ifeq ($(GUI), QT4)
   ifneq ($(USES_QT4),)
    QMAKE       = ${shell which qmake-qt4 2>/dev/null}
    ifeq ($(QMAKE),)
      QMAKE = ${shell which qmake 2>/dev/null}
    endif
    ifeq ($(QMAKE),)
      $(error qmake from Qt not found! Make sure the Qt binaries are in your PATH)
    endif
    MOC = $(shell which moc-qt4 2>/dev/null)
    ifeq ($(MOC),)
      MOC = $(shell which moc 2>/dev/null)
    endif
    ifeq ($(MOC),)
      $(error moc from Qt not found! Make sure the Qt binaries are in your PATH)
    endif
    UIC         = $(shell which uic-qt4 2>/dev/null)
    ifeq ($(UIC),)
      UIC = $(shell which uic 2>/dev/null)
    endif
    ifeq ($(UIC),)
      $(error uic from Qt not found! Make sure the Qt binaries are in your PATH)
    endif
    RCC         = $(shell which rcc-qt4 2>/dev/null)
    ifeq ($(RCC),)
      RCC = $(shell which rcc 2>/dev/null)
    endif
    ifeq ($(RCC),)
      $(error rcc from Qt not found! Make sure the Qt binaries are in your PATH)
    endif
    LRELEASE    = $(shell which lrelease-qt4 2>/dev/null)
    ifeq ($(LRELEASE),)
      LRELEASE = $(shell which lrelease 2>/dev/null)
    endif
    ifeq ($(LRELEASE),)
      $(error lrelease from Qt not found! Make sure the Qt binaries are in your PATH)
    endif
    QT_FLAGS    = $(shell pkg-config QtCore QtGui --cflags)
    QT_LIBS     = $(shell pkg-config QtCore QtGui --libs)
    # define Gtk flags when using Qt4 gui so it can load plugins, etc.
  endif
endif

# set base program pointers and flags
CC      = gcc
CXX     = g++
LD      = g++
ifeq ($(OS),LINUX)
STRIP	= strip -s
endif
ifeq ($(OS),OSX)
STRIP	= strip -x 
endif
RM      = rm
RM_F    = rm -f
MV      = mv
CP      = cp
MD      = mkdir
FIND    = find
PROF    = gprof
INSTALL = ginstall

# create SVN version defines
MUPEN_RELEASE = 1.5

ifneq ($(RELEASE),)
  MUPEN_VERSION = $(VER)
  PLUGIN_VERSION = $(VER)
else 
  ifeq ($(shell svn info ./ 2>/dev/null),)
    MUPEN_VERSION = $(MUPEN_RELEASE)-development
    PLUGIN_VERSION = $(MUPEN_RELEASE)-development
  else
    SVN_REVISION = $(shell svn info ./ 2>/dev/null | sed -n '/^Revision: /s/^Revision: //p')
    SVN_BRANCH = $(shell svn info ./ 2>/dev/null | sed -n '/^URL: /s/.*mupen64plus.//1p')
    SVN_DIFFHASH = $(shell svn diff ./ 2>/dev/null | md5sum | sed '/.*/s/ -//;/^d41d8cd98f00b204e9800998ecf8427e/d')
    MUPEN_VERSION = $(MUPEN_RELEASE)-$(SVN_BRANCH)-r$(SVN_REVISION) $(SVN_DIFFHASH)
    PLUGIN_VERSION = $(MUPEN_RELEASE)-$(SVN_BRANCH)-r$(SVN_REVISION)
  endif
endif

# set base CFLAGS and LDFLAGS
CFLAGS += -pipe -O3 -ffast-math -funroll-loops -fexpensive-optimizations -fno-strict-aliasing
CORE_LDFLAGS += -lz -lm -lpng -lfreetype -ldl

# set special flags per-system
ifeq ($(CPU), X86)
  ifeq ($(ARCH), 64BITS)
    CFLAGS += -march=athlon64
  else
    CFLAGS += -march=i686 -mtune=pentium-m -mmmx -msse
    ifneq ($(PROFILE), 1)
      CFLAGS += -fomit-frame-pointer
    endif
  endif
  # tweak flags for 32-bit build on 64-bit system
  ifeq ($(ARCH), 64BITS_32)
    CFLAGS += -m32
    LDFLAGS += -m32 -m elf_i386
  endif
endif
ifeq ($(CPU), PPC)
  CFLAGS += -mcpu=powerpc -D_BIG_ENDIAN
endif

# set CFLAGS, LIBS, and LDFLAGS for external dependencies
ifeq ($(OS),LINUX)
  PLUGIN_LDFLAGS	= -Wl,-Bsymbolic -shared
endif
ifeq ($(OS),OSX)
  PLUGIN_LDFLAGS	= -bundle
endif

ifeq ($(OS),LINUX)
  LIBGL_LIBS	= -L/usr/X11R6/lib -lGL -lGLU
endif
ifeq ($(OS),OSX)
  LIBGL_LIBS	= -framework OpenGL
endif

# set flags for compile options.

# set CFLAGS macro for no assembly language if required
ifeq ($(NO_ASM), 1)
  CFLAGS += -DNO_ASM
endif

# set variables for profiling
ifeq ($(PROFILE), 1)
  CFLAGS += -pg -g
  LDFLAGS += -pg
  STRIP = true
else   # set variables for debugging symbols
  ifeq ($(DBGSYM), 1)
    CFLAGS += -g
    STRIP = true
  endif
endif

SO_EXTENSION = so

