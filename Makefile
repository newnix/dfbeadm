.POSIX:

## Program specs ##
SRC = dfbeadm.c fscollect.c fstest.c fsupdate.c fslist.c snapfs.c fsrecord.c
TARGET = dfbeadm

## Some environmental info for installation ##
MUSER = ${USER}
GROUP = ${USER}
PREFIX = ${HOME}
DESTDIR = /bin/
MODE = 0755

## Include files and Libraries to link ##
INCS = -I. -I/usr/include
LIBS = -L. -L/usr/lib

## Compilation flags ##
DBG = gdb 
DBGFLAGS = -g${DBG} -DDEBUG -DTIMING

CFLAGS = -Wall -Wextra -Weverything -pedantic -std=c99 -Oz\
				  -z now -z combreloc\
				 -Wparentheses -ffast-math -z relro -pipe -fvectorize -fstack-protector \
				 -fstrict-enums -fstrict-return -fstack-protector-strong -fsanitize-cfi-cross-dso \
				 -fmerge-all-constants -fstack-protector-all -Qn \
				 -Wl,${LDFLAGS} -pipe -fstrict-aliasing -fuse-ld=${LD}

LDFLAGS = --gc-sections,--icf=all,--print-icf-sections,--print-gc-sections

## Which compiler and linker to use ##
CC = clang-devel
LD = lld-devel

## How to get help/usage output from the binary ##
HELP = -h

help:
	@printf "Build configuration for ${TARGET}:\n"
	@printf "\nPREFIX:\t%s\nDIR:\t%s\nINST:\t%s\nOWNER:\t%s\nGROUP:\t%s\nMODE:\t%s\n\nCC:\t%s\nLD:\t%s\nCFLAGS:\t%s\nINCS:\t%s\nLIBS:\t%s\n"\
		"${PREFIX}" "${DESTDIR}" "${PREFIX}${DESTDIR}${TARGET}" "${MUSER}" "${GROUP}" "${MODE}" "${CC}" "${LD}" "${CFLAGS}" "${INCS}" "${LIBS}"
	@printf "\n\nChange these settings with %s %s\n" ${EDITOR} "defaults.mk"
	@printf "Valid targets: build, debug, help, install, uninstall, rebuild, reinstall, run\n"

build: ${SRC}
	$(CC) -o $(TARGET) $(CFLAGS) $(INCS) $(LIBS) $?

build-dbg: ${SRC}
	$(CC) -o $(TARGET) $(CFLAGS) $(DBGFLAGS) $(INCS) $(LIBS) $?

check: ${SRC}
	#clang-check-devel -analyze ${SRC}
	clang-tidy-devel $?

debug: build-dbg
	@mkdir -p ${PREFIX}${DESTDIR}
	@chown ${USER}:${GROUP} ${TARGET}
	@chmod ${MODE} ${TARGET}
	@cp -fp ${TARGET} ${PREFIX}${DESTDIR}
	@rm -f ${TARGET}
	@printf "\n\n"
	$(PREFIX)$(DESTDIR)$(TARGET) $(HELP)

install: build
	@mkdir -p ${PREFIX}${DESTDIR}
	@chown ${USER}:${GROUP} ${TARGET}
	@chmod ${MODE} ${TARGET}
	@cp -fp ${TARGET} ${PREFIX}${DESTDIR}
	@rm -f ${TARGET}
	@printf "\n\n"
	$(PREFIX)$(DESTDIR)$(TARGET) $(HELP)

uninstall: ${PREFIX}${DESTDIR}${TARGET}
	@rm -f ${PREFIX}${DESTDIR}${TARGET}

rebuild: uninstall debug

reinstall: uninstall install

clean: uninstall

push:
	@gitsync -r ${TARGET} -n v0.1.0-BETA

run: ${PREFIX}${DESTDIR}${TARGET}
	$(PREFIX)$(DESTDIR)$(TARGET)
