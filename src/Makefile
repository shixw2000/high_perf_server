release=

RM=rm -f

COMILE_OPT=-Wno-unused-parameter
MACROS=-D__USE_MB__ -D__UNUSE_TIMER__N \
        -D__USE_UNIX__N 

ifndef release
CC=g++ -g -W -Wall -D__LOG_LEVEL__=3 $(MACROS) $(COMILE_OPT)
else
#-s :删除可执行程序中的所有符号表和所有重定位信息。其结果与运行命令 strip 所达到的效果相同
CC=g++ -s -O3 -W -Wall -D__LOG_LEVEL__=2 $(MACROS) $(COMILE_OPT)
endif


prog:=bin_test
inc_dir=
lib_dir=
libs=-lpthread

srcs=client.cpp server.cpp socktool.cpp sockpool.cpp sockdata.cpp \
	cthread.cpp hlist.cpp receiver.cpp sender.cpp \
	transhandler.cpp worker.cpp taskhandler.cpp pushpool.cpp \
	service.cpp statemachine.cpp sockmonitor.cpp task.cpp \
	msgdealer.cpp msgpool.cpp lock.cpp hque.cpp director.cpp \
	sockdeal.cpp sockconf.cpp
	
objs=$(srcs:.cpp=.o)

default:$(prog)
.PHONY:default

$(prog):$(objs)
	$(CC) -o $@ $^ $(lib_dir) $(libs)

$(objs):%.o:%.cpp
	$(CC) -c -o $@ $< $(inc_dir)	

clean:
	-@$(RM)  $(objs) $(prog)
.PHONY:clean


