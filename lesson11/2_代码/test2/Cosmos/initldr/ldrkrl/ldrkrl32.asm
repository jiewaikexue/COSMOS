

;============================================
; 本文件重点知识
; 1. 为什么需要重新 加载GDT and IDT
; 2. 二级引导提供BIOS 中断服务
; 3. 二级引导器,的功能主要是 搜集信息
;     --> 内存信息的获取 是 依靠 15号中断的
;     --> 通过查看getmmap的汇编代码
; 4. 要在二级引导器搜集那些信息
;       1. 内存布局信息
;       2. 设置显卡图像模式
; 5. BIOS工作模式: 实模式
; 6. C语言工作模式: 保护模式,虚拟内存地址
; 7. 如何在c语言环境下 调用BIOS中断(夸模式中断)  ==============> 实模式: 工作在16位  c语言编译器一般编译的都是 32位
;                                             --------------> 需要 在32位的 c转汇编代码 执行 bios中断
;    ====>  1. 保存 C 语言环境下的 CPU 上下文 ，即保护模式下的所有通用寄存器、段寄存器、程序指针寄存器，栈寄存器，把它们都保存在内存中。2. 切换回实模式，调用 BIOS 中断，把 BIOS 中断返回的相关结果，保存在内存中。3. 切换回保护模式，重新加载第 1 步中保存的寄存器。这样 C 语言代码才能重新恢复执行。
;           2. 切换回实模式，调用 BIOS 中断，把 BIOS 中断返回的相关结果，保存在内存中。3. 切换回保护模式，重新加载第 1 步中保存的寄存器。这样 C 语言代码才能重新恢复执行。
;           3. 切换回保护模式，重新加载第 1 步中保存的寄存器。这样 C 语言代码才能重新恢复执行。
;							这种说法不太妥当吗，容易引起混淆： 
;							并不是说C语言代码工作在32位保护模式，C代码最终是要转换成汇编代码的，
;							系统最开始是工作在16位实模式，通过CR0寄存器设置标志而转换成保护模式。
;							那么，工作在16位实模式下，就不能用C语言来编写了吗？ 
;							实际并不是的，C最终是要转换成汇编代码的，只不过编译器支持编程位16位模式。 
;							最初版本的gcc编译器是支持的，后来的版本就全部改成了32模式了。
;							 这个历史和概念的划分要弄明白，不然容易陷进去。
;============================================

%include "ldrasm.inc"
global _start
global realadr_call_entry
global IDT_PTR
extern ldrkrl_entry
[section .text]
[bits 32]
_start:
_entry:
	cli ; 关闭中断
; ++++++++++++++++++++++++++++++++++++++++++++++++++++
; 为什么代码模块改变后就要对寄存器做重新的初始化呢？
;
; 所谓“切换代码模块”的意思是，我们有了新的运行信息，需要针对这些变化做一些更新工作。
; 以这里为例，GDT 和 IDT 的内容变了。
; IDT 之前没有，可以一眼看出来 
; GDT 有了变化，其实注释写了，但我一开始也没反应过来（坏注释，要避免）。
; 注释里的 a-e 的意思是，原来是 9e 的那个部分，变为 9a 了。
; ++++++++++++++++++++++++++++++++++++++++++++++++++++
	lgdt [GDT_PTR] ; 加载GDT地址到GDTR寄存器
	lidt [IDT_PTR] ; 加载IDT地址 到IDTR寄存器
	jmp dword 0x8 :_32bits_mode ; 长跳转,刷新影子寄存器

_32bits_mode:
	mov ax, 0x10	; 数据段选择子(目的)
	mov ds, ax
	mov ss, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
    ; 异或初始化寄存器
	xor eax,eax
	xor ebx,ebx
	xor ecx,ecx
	xor edx,edx
	xor edi,edi
	xor esi,esi
	xor ebp,ebp
	xor esp,esp
	mov esp,0x90000 ; 使得栈底指向了0x90000
	call ldrkrl_entry ;调用ldrkrl_entry函数
	xor ebx,ebx
	jmp 0x2000000 ; 跳转到0x2000000的真实物理内存地址
	jmp $


realadr_call_entry:
	pushad  ;保存通用寄存器信息
	push    ds
	push    es
	push    fs ;保存4个段寄存器
	push    gs
	call save_eip_jmp ;调用save_eip_jmp
	pop	gs
	pop	fs
	pop	es  ;恢复4个段寄存器
	pop	ds
	popad ; 恢复通用寄存器
	ret
save_eip_jmp:
;====================================
;  1.EIP寄存器里存储的是CPU下次要执行的指令的地址。 ========> 也就是调用完fun函数后，让CPU知道应该执行main函数中的printf（“函数调用结束”）语句了。
;  2.EBP寄存器里存储的是是栈的栈底指针，通常叫栈基址，这个是一开始进行fun()函数调用之前，由ESP传递给EBP的。（在函数调用前你可以这么理解：ESP存储的是栈顶地址，也是栈底地址。）
;  3.ESP寄存器里存储的是在调用函数fun()之后，栈的栈顶。并且始终指向栈顶。
;  4. esi: 通用寄存器
;====================================

	pop esi ; 弹出call save_eip_jmp时保存的eip到esi寄存器中， 
	mov [PM32_EIP_OFF],esi ;把eip保存到特定的内存空间中
	mov [PM32_ESP_OFF],esp ;把esi保存到特定的内存空间中

	;	在这里 进行  
	jmp dword far [cpmty_mode] ; 长跳转这里表示把cpmty_mode处的第一个4字节装入eip，把其后的2字节装入cs
cpmty_mode:
;================================================
;+	0x18=0000 0000 0001 1000 0000 0000 0 0011 0 00 
;+	TI=0 RPL=0
;+	描述符在GDT中的序号：3 
;+	我们在前面建立的GDT表  索引为3的描述符就是16位  代码段  描述符。
;+	GDT_START: 
;+		knull_dsc: dq 0 
;+		kcode_dsc: dq 0x00cf9e000000ffff 
;+		kdata_dsc: dq 0x00cf92000000ffff 
;+		k16cd_dsc: dq 0x00009e000000ffff ;16位代码段描述符 
;+		k16da_dsc: dq 0x000092000000ffff ;16位数据段描述符 
;+	GDT_END:
;================================================

	dd 0x1000 ; 这个地址是不是 很熟悉 就是 initldrkrlsve.bin的装载的真实物理地址
	dw 0x18 ; 指向GDT表的代码段 , 
	jmp $

;================================================
;	一下是16位模式下的GDT表
;		数据段: 0x20
;		代码段: 0x18
;================================================

GDT_START:
knull_dsc: dq 0
kcode_dsc: dq 0x00cf9a000000ffff ; 原来是 9e 现在是 9a
kdata_dsc: dq 0x00cf92000000ffff
k16cd_dsc: dq 0x00009a000000ffff ; 原来是 9e 现在是 9a code 代码段
k16da_dsc: dq 0x000092000000ffff ; data数据段
GDT_END:

GDT_PTR:
GDTLEN	dw GDT_END-GDT_START-1	;GDT界限
GDTBASE	dd GDT_START

IDT_PTR:
IDTLEN	dw 0x3ff
IDTBAS	dd 0 ;这是BIos 中断表的地址 and长度
