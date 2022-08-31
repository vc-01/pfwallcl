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
 dgclock.obj \
 graph.obj \
 pfbios.obj \
 pfwallcl.obj \
 timer.obj

#		*Explicit Rules*
build\pfwallcl.exe: pfwallcl.cfg $(EXE_dependencies)
  $(TLINK) /x/c/d/L$(LIBPATH) @&&|
c0s.obj+
build\fnt_dat.obj+
build\dgclock.obj+
build\graph.obj+
build\pfbios.obj+
build\pfwallcl.obj+
build\timer.obj
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

dgclock.obj: pfwallcl.cfg src\dgclock.cpp 
	$(CC) -c src\dgclock.cpp

graph.obj: pfwallcl.cfg src\graph.cpp 
	$(CC) -c src\graph.cpp

pfbios.obj: pfwallcl.cfg src\pfbios.cpp 
	$(CC) -c src\pfbios.cpp

pfwallcl.obj: pfwallcl.cfg src\pfwallcl.cpp 
	$(CC) -c src\pfwallcl.cpp

timer.obj: pfwallcl.cfg src\timer.cpp 
	$(CC) -c src\timer.cpp

#		*Compiler Configuration File*
pfwallcl.cfg: pfwallcl.mak
  copy &&|
-a
-G
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
-DNTVDM_;SSHOT_
| pfwallcl.cfg


