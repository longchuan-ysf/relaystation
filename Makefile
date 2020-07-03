CROSS_COMPILE 	?= arm-linux-gnueabihf-
TARGET		  	?= relaystation

CC 				:= $(CROSS_COMPILE)gcc

LIBPATH			:= -lgcc -lpthread -lzlog -L /home/longchuan/linux/zlog/zlog/build_arm/lib/

INCDIRS 		:= 	project \
					EquipmentManage \
					TCPBackground \
					TCPModbus \
					comfunction \
					record \
					log
SRCDIRS			:= 	project \
					comfunction \
					EquipmentManage \
					TCPBackground \
					TCPModbus \
					record \
					log


				   			   
INCLUDE			:= $(patsubst %,-I %, $(INCDIRS))

SFILES			:= $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.S))
CFILES			:= $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.c))


SFILENDIR		:= $(notdir  $(SFILES))
CFILENDIR		:= $(notdir  $(CFILES))

SOBJS			:= $(patsubst %, obj/%, $(SFILENDIR:.S=.o))
COBJS			:= $(patsubst %, obj/%, $(CFILENDIR:.c=.o))
OBJS			:= $(SOBJS) $(COBJS)

VPATH			:= $(SRCDIRS)
#include 路径追加zlog包含路径
INCLUDE 		+= -I /home/longchuan/linux/zlog/zlog/build_arm/include

.PHONY: clean
	
$(TARGET): $(OBJS)#将项目中的.c .S文件编译的.o链接成最终可执行文件
	$(CC) $(OBJS) -o $(TARGET) $(LIBPATH)
$(SOBJS) : obj/%.o : %.S#编译所有.S文件
	$(CC) -Wall -Werror -nostdlib -fno-builtin -c -O2 -o $@ $< $(INCLUDE)

$(COBJS) : obj/%.o : %.c#编译所有.c文件
	$(CC) -Wall -nostdlib -fno-builtin  -c -O2 -o $@ $< $(INCLUDE)
	
clean:
	rm -rf $(TARGET) $(COBJS) $(SOBJS)

	
