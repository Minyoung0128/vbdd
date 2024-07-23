savedcmd_/usr/src/linux-6.9.5/vbdd/csl.mod := printf '%s\n'   csl_main.o backup.o | awk '!x[$$0]++ { print("/usr/src/linux-6.9.5/vbdd/"$$0) }' > /usr/src/linux-6.9.5/vbdd/csl.mod
