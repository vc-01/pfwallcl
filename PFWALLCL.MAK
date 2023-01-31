.AUTODEPEND

.PATH.obj = BUILD

#		*Translator Definitions*
CC = bcc +PFWALLCL.CFG
TASM = TASM
TLIB = tlib
TLINK = tlink
LIBPATH = C:\BORLANDC\LIB
INCLUDEPATH = C:\BORLANDC\INCLUDE;SRC


#		*Implicit Rules*
.c.obj:
  $(CC) -c {$< }

.cpp.obj:
  $(CC) -c {$< }

#		*List Macros*


EXE_dependencies =  \
 fnt_dat.obj \
 graph.obj \
 dgclock.obj \
 timer.obj \
 timer_dt.obj \
 fixedp.obj \
 inifile.obj \
 pfbios.obj \
 pfwallcl.obj

#		*Explicit Rules*
build\pfwallcl.exe: pfwallcl.cfg $(EXE_dependencies)
  $(TLINK) /x/c/d/s/L$(LIBPATH) @&&|
c0s.obj+
build\fnt_dat.obj+
build\graph.obj+
build\dgclock.obj+
build\timer.obj+
build\timer_dt.obj+
build\fixedp.obj+
build\inifile.obj+
build\pfbios.obj+
build\pfwallcl.obj
build\pfwallcl
		# no map file
graphics.lib+
emu.lib+
maths.lib+
cs.lib
|


#		*Individual File Dependencies*
fnt_dat.obj: pfwallcl.cfg src\fnt_dat.asm
	$(TASM) /MX /ZI /O SRC\FNT_DAT.ASM,BUILD\FNT_DAT.OBJ

graph.obj: pfwallcl.cfg src\graph.cpp
	$(CC) -c src\graph.cpp

dgclock.obj: pfwallcl.cfg src\dgclock.cpp
	$(CC) -c src\dgclock.cpp

timer.obj: pfwallcl.cfg src\timer.cpp
	$(CC) -c src\timer.cpp

timer_dt.obj: pfwallcl.cfg src\timer_dt.cpp
	$(CC) -c src\timer_dt.cpp

fixedp.obj: pfwallcl.cfg src\fixedp.cpp
	$(CC) -c src\fixedp.cpp

inifile.obj: pfwallcl.cfg src\inifile.cpp
	$(CC) -c src\inifile.cpp

pfbios.obj: pfwallcl.cfg src\pfbios.cpp
	$(CC) -c src\pfbios.cpp

pfwallcl.obj: pfwallcl.cfg src\pfwallcl.cpp
	$(CC) -c src\pfwallcl.cpp

#		*Compiler Configuration File*
pfwallcl.cfg: pfwallcl.mak
  copy &&|
-a
-G
-1-
-f-
-O
-Og
-Oe
-Om
-Ov
-Ol
-Ob
-Op
-Oi
-Vmd
-Vc
-Va
-Vt
-Z
-k-
-d
-h
-S
-B
-wpro
-weas
-wpre
-nBUILD
-I$(INCLUDEPATH)
-L$(LIBPATH)
-DNTVDM_;EMUFPU_;SSHOT_;TESTS_
| pfwallcl.cfg
