all: client.c libmfs.c server.c udp.h udp.c mfs.h ufs.h mkfs.c 
	gcc server.c udp.c -o server
	gcc -c -Wall -fpic libmfs.c udp.c
	gcc -shared -o libmfs.so libmfs.o
	gcc client.c udp.c -o client -L. -lmfs
	gcc mkfs.c -o mkfs




# CC     := gcc

# CFLAGS := -Wall -Werror 
# IMGFGS := -f -i -d 
# SRCS   := server.c \
# #	  mfs.c \
# #	  mkfs.c \
# #	  client.c

# #LD_LIBRARY_PATH:=/home/nickolas/537_Proj/p4
# LD_LIBRARY_PATH:= . 
# export LD_LIBRARY_PATH

# OBJS   := ${SRCS:c=o}
# PROGS  := ${SRCS:.c=}
# LIBPTH := LD_LIBRARY_PATH

# INODES := 32
# DATA   := 32
# IMAGE  := image
# PORT   := 10000
# CLIENTPORT := 10000

# .PHONY: all
# all: ${PROGS} obj lib exe mkfs 

# ${PROGS} : % : %.o Makefile
# 	${CC} $< -o $@ udp.c
# run:
# 	server ${PORT} ${IMAGE}
# clean:
# 	rm -f ${PROGS} ${OBJS}
# 	rm udp.o
# 	rm mfs.o
# 	rm *.so
# 	rm client
# #	rm mkfs
# conn:
# 	./client ${CLIENTPORT} ${IMAGE}
# %.o: %.c Makefile
# 	${CC} -c $<
# mkfsrun: 
# 	./mkfs -f ${IMAGE} -i ${INODES} -d ${DATA} 
# mkfs: mkfs.c
# 	${CC} -o mkfs mkfs.c
# exe: libmfs.so
# 	gcc -L. -o client client.c -Wall libmfs.so
# lib: mfs.o udp.o
# 	gcc -shared -o libmfs.so mfs.o udp.o

# obj: mfs.c udp.c server.c
# 	gcc -c -fpic mfs.c 
# 	gcc -c -fpic udp.c
# 	gcc -c -fpic server.c

# test:
# 	sh ~cs537-1/tests/p4/p4-test/runtests.sh -t 1
# testall:
# 	sh ~cs537-1/tests/p4/p4-test/runtests.sh -c