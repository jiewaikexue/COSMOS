# jie @ 2022-10-23 16:05:24

MAKEFLAGS = -sR # 
# =========== 命令的别名 =============
MKDIR = mkdir 
RMDIR = rmdir
CP = cp 
CD = cd
DD = dd
RM = rm

ASM		= nasm
CC		= gcc
LD		= ld
OBJCOPY	= objcopy
# =========== 命令的别名 =============

# =========== nasm 参数 =============
# 中文手册:
#			https://www.docin.com/p-2372955501.html
# -f :	用来指定编译出来的.o文件的格式。
# 	 ==> nasm -hf来查看： 支持的所有格式
# -w :使得特定的汇编警告信息无效化
# -w-orphan-labels: 禁止  源文件中没有指令,缺定义了一个有结尾分号的 lable
# =========== nasm 参数 =============
ASMBFLAGS	= -f elf -w-orphan-labels

# =========== gcc 参数 ==============
# -Os : Os支持通常不会增加代码大小的全O2优化。
# -std=c99 : 采用c99标准
# -m32 : -m32选项设置“int”、“long”和指针类型 ,并生成在任何i386系统上运行的代码。
# -Wall : 打开所有gcc能够提供的，常用的警告信息。
# -Wshadow : 用于 局部作用域内 局部变量 遮蔽同名的全局变量时 ,会给出警告
# ... 相关信息
# =========== gcc 参数 ==============
CFLAGS		= -c -Os -std=c99 -m32 -Wall -Wshadow -W -Wconversion -Wno-sign-conversion  -fno-stack-protector -fomit-frame-pointer -fno-builtin -fno-common  -ffreestanding  -Wno-unused-parameter -Wunused-variable


# =========== LD 参数 ===============
# -s :去掉文件中的全部符号信息
# -static: 使用静态库
# -T: 指定hello.lds座位链接器脚本
# -n : 	
# 		1. 关闭 "页面对齐"
#		2. 禁用 对共享库的链接
# 		3. 如果输出格式支持UNIX的幻数 ,则会将输出标记成 NMAGIC
# -Map: 将链接映射输出成为指定地文件 -> hellos.map
# =========== LD 参数 ===============
LDFLAGS		= -s -static -T hello.lds -n -Map HelloOS.map 


# =========== objcopy参数说明 ==========
# -S: 不从源文件拷贝 符号信息和 relocation信息
# -O:指定输出文件的格式  -->2进制格式
# =========== objcopy参数说明 ==========
OJCYFLAGS	= -S -O binary

# 宏赋值为空
HELLOOS_OBJS := 
#
HELLOOS_OBJS += entry.o main.o vgastr.o
HELLOOS_ELF = HelloOS.elf
HELLOOS_BIN = HelloOS.bin
# 伪目标
.PHONY : build clean all link bin

all: clean build link bin

clean:
	$(RM) -f *.o *.bin *.elf

build: $(HELLOOS_OBJS)

link: $(HELLOOS_ELF)
$(HELLOOS_ELF): $(HELLOOS_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(HELLOOS_OBJS)
bin: $(HELLOOS_BIN)
$(HELLOOS_BIN): $(HELLOOS_ELF)
	$(OBJCOPY) $(OJCYFLAGS) $< $@

%.o : %.asm
	$(ASM) $(ASMBFLAGS) -o $@ $<
%.o : %.c
	$(CC) $(CFLAGS) -o $@ $<


