
RM=rm -f

VERSION:=$(shell date "+%F_%T")
COMILE_OPT=
MACROS=-D__BUILD_VER__="\"V1.0 $(VERSION)\""

ifdef debug
CC=g++ -g -W -Wall $(MACROS) $(COMILE_OPT)
else
#-s :strip symbols 
CC=g++ -s -O3 -W -Wall $(MACROS) $(COMILE_OPT)
endif


prog:=bin_test
inc_dir=
lib_dir=
libs=-lpthread -lrt

srcs=$(wildcard *.cpp) 
	
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


