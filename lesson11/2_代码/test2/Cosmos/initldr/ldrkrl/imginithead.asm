; ===============================================================
; GRUB的头部汇编部分
; GRUB 头部 
;   1. imginithead.asm: GRUB头部汇编部分
;           --> 主要功能: 初始化 CPU 的寄存器，加载 GDT，切换到 CPU 的保护模式，
;   2. inithead.c :主要功能是 查找二级引导器的核心文件 -- initldrkrl.bin,然后将其放在特定位置
; 本文件: GRUB 头部汇编
; 回顾:  cpu在第一次加电 和bios刚刚运行完毕之前, 一直都是实模式
; ===============================================================



; - MBT_HDR_FLAGS EQU 0x00010003  : 我们设置的 flag  
;   |---------> 2进制的第16位被置为 1
;   |---------> 因此load_addr和entry_addr都是有效的，而它们正好分别对应_start和_entry。 
;                     |----------> load_addr:是引导器二进制文件text段的起始地址，即_start，grub解析头部数据后，拿到_start地址，并从该地址处开始执行二级引导器代码。 
;                     |----------> entry_addr : 对应的是操作系统的入口点，也就是_entry。引导程序最后将跳转到这里
;                                         |----------->   不过本文的实现并没有完全按照这种思路来，_start直接跳到_entry，然后由_entry负责二级引导工作。

; EQU 指令类似常
MBT_HDR_FLAGS  EQU 0x00010003 
MBT_HDR_MAGIC  EQU 0x1BADB002 ; 多引导协议头 魔数字 GRUB
MBT2_MAGIC  EQU 0xe85250d6 ; 第二版 多引导 协议头 魔数字  GRUB2



; global: 定义一个全局可见的变量
global _start  ;定义一个全局可见的变量 _start
; extern:引入外部的符号
extern inithead_entry ; inithead_entry 是外部引入的符号
; section: 指定节(ELF)
[section .text] ; 后续代码是 代码节的
[bits 32] ; 代码运行环境是 32位机器
; 汇编程序代码入口 : _start
_start:
  jmp _entry ; 跳转到 _entry
align 4 ; 地址对齐 4字节


;=====================================
;  mbt_hdr : 创建 GRUB 所需要的 head
;=====================================
mbt_hdr:
  dd MBT_HDR_MAGIC
  dd MBT_HDR_FLAGS
  dd -(MBT_HDR_MAGIC+MBT_HDR_FLAGS)
  dd mbt_hdr
  dd _start
  dd 0
  dd 0
  dd _entry
ALIGN 8 ;地址对齐 8字节

;====================================
;   mbhdr: 创建GRUB2 所需要的的head
;      包含两个头是为了同时兼容GRUB、GRUB2
;====================================
mbhdr:
  DD  0xE85250D6
  DD  0
  DD  mhdrend - mbhdr
  DD  -(0xE85250D6 + 0 + (mhdrend - mbhdr))
  DW  2, 0
  DD  24
  DD  mbhdr
  DD  _start
  DD  0
  DD  0
  DW  3, 0
  DD  12
  DD  _entry 
  DD  0  
  DW  0, 0
  DD  8
mhdrend:


_entry:

;==================================
;中断: 
;  1. 可屏蔽中断: cli
;  2. 不可屏蔽中断: eg 掉电 等...
;        中断标志位 清零
;==================================

; cli: 禁止中断发生  sti: 恢复中断发生
	cli


;==================================
;  利用al 彻底关闭 不可屏蔽的中断
;
;  读取CMOS/RTC地址，设置最高位为1，写入0x70端口，关闭不可屏蔽中断!
;==================================

; in :将指定的端口的数据 读入
	in al, 0x70 ; 将0x70的数据读入 al
; OR 指令在两个操作数的对应位之间进行（按位）逻辑或（OR）操作，并将结果存放在目标操作数中：
	or al, 0x80	
; out : 将 al的数据 写入到 0x70
	out 0x70,al

;==================================
;    2022年10月31日17:16:40: 注意,这里并没有彻底的从实模式,切换到保护模式
;                            ===> 只是单纯的创建了 GDT 
;                            ===> 并没有说明 and 设置 特权级别 (R0 ~ R3)
;==================================
	lgdt [GDT_PTR] ;加载GDT地址到GDTR寄存器
	jmp dword 0x8 :_32bits_mode ; 长跳转刷新CS影子寄存器

;==================================
;_32bits_mode:
;            初始化段寄存器和通用寄存器、栈寄存器，
;            这是为了给调用 inithead_entry 这个 C函数做准备
;==================================

_32bits_mode:
	mov ax, 0x10
	mov ds, ax
	mov ss, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
  ; 全部异或,全部置为0
	xor eax,eax
	xor ebx,ebx
	xor ecx,ecx
	xor edx,edx
	xor edi,edi
	xor esi,esi
	xor ebp,ebp
	xor esp,esp
	mov esp,0x7c00 ; 设置内存空间 空栈的栈顶
  ; 调用 inithead_entery 方法 
	call inithead_entry ; 该函数 在本文件中并没有实现, 在外部 inithead.c 中实现
	jmp 0x200000 ; 跳转到 该地址 (真实物理地址)



;  GDT: 全局段 描述符表
GDT_START:
knull_dsc: dq 0
kcode_dsc: dq 0x00cf9e000000ffff
kdata_dsc: dq 0x00cf92000000ffff
k16cd_dsc: dq 0x00009e000000ffff ; 16位代码段描述符
k16da_dsc: dq 0x000092000000ffff ; 16位数据段描述符
GDT_END:
GDT_PTR:
GDTLEN	dw GDT_END-GDT_START-1	;GDT界限
GDTBASE	dd GDT_START ; GDT的基地址
