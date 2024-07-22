NAME = csl

SOURCES = csl_main.c backup.c

# obj-m에 객체 파일을 추가
obj-m += ${NAME}.o

# 각각의 객체 파일을 모듈에 추가
${NAME}-objs := $(patsubst %.c,%.o,${SOURCES})

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f Module.symvers modules.order
	rm -f ${SOURCES:.c=.o} ${SOURCES:.c=.mod} ${SOURCES:.c=.mod.c} ${SOURCES:.c=.mod.o}

fclean: clean
	rm -f ${NAME}.ko

re: fclean all

