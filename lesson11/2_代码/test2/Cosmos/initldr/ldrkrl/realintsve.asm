;=============================================
;  本文件 编译最终 形成  initldrsve.bin 文件
;  存放位置: 0x1000
;  工作内容: 获取内存布局信息 ,设置显卡工作模式 
;  	    dw _getmmap ;获取内存布局视图的函数
;  	    dw _read ;读取硬盘的函数
;          dw _getvbemode ; 获取显卡VBE模式
;          dw _getvbeonemodeinfo ;获取显卡VBE 模式的数据
;          dw _setvbemode ;设置显卡VBE模式
;=============================================

%include "ldrasm.inc"
global _start
[section .text]
[bits 16]
_start:
_16_mode:
	mov	bp,0x20 ; 0x20 是GDT中 16位数据段的描述符
	mov	ds, bp
	mov	es, bp
	mov	ss, bp
	mov	ebp, cr0 ;CR0 寄存器 存放 特权级别
	and	ebp, 0xfffffffe
	mov	cr0, ebp ;CR0=0 关闭了保护模式
	jmp	0:real_entry ; 刷新影子寄存器,进入到真正的实模式
real_entry:
	mov bp, cs
	mov ds, bp
	mov es, bp
	mov ss, bp ;冲洗设置实模式下的段寄存器,都是CS中的值,即是0
	mov sp, 08000h ;设置栈
	mov bp,func_table ; 函数表交给 bp
	add bp,ax ;ax是C函数的传入的参数 , 根据real_entery传入的参数选择调用函数
	call [bp] ;执行函数表中的汇编函数,
	cli ; 关中断
	call disable_nmi 
	mov	ebp, cr0
	or	ebp, 1
	mov	cr0, ebp ;CR0 = 1 关闭实模式
	jmp dword 0x8 :_32bits_mode ; 执行长跳转,刷新影子寄存器, 真正切换到 保护模式
[BITS 32]
_32bits_mode: ;当前从16bit 返回到了 32位
	mov bp, 0x10
	mov ds, bp
	mov ss, bp ;重新设置保护模式下的段寄存器0x10是32位数据段描述符的索引
	mov esi,[PM32_EIP_OFF] ;加载先前保存的 EIP
	mov esp,[PM32_ESP_OFF] ;加载先前保存的 ESP
;=========================================================
;       最后返回32位保护模式，恢复进入前保存的EIS+EIP返回调用前的32位保护模式状态。
;       回到了 ldrkrl32.asm -> realadr_call_entry 中
;=========================================================
	jmp esi ;

[BITS 16]
DispStr:
	mov bp,ax
	mov cx,23
	mov ax,01301h
	mov bx,000ch
	mov dh,10
	mov dl,25
	mov bl,15
	int 10h
	ret
cleardisp:
	mov ax,0600h     	;这段代码是为了清屏
	mov bx,0700h
	mov cx,0
	mov dx,0184fh
	int 10h			;调用的BOIS的10号
	ret

_getmmap:
	push ds
	push es
	push ss
	mov esi,0
	mov dword[E80MAP_NR],esi
	mov dword[E80MAP_ADRADR],E80MAP_ADR

	xor ebx,ebx ;ebx设为0
	mov edi,E80MAP_ADR ;edi设为存放输出结果的1MB内的物理内存地址
loop:
;==================================
;	是使用INT 15时对AX寄存器指定的值。用来获取系统内存map 
;	参考: http://www.ctyme.com/intr/int-15.htm 
;		http://www.ctyme.com/intr/rb-1741.htm
;==================================
	mov eax,0e820h ;eax 必须为0e820h
	
	
;==================================
;		输出结果数据项的大小为20字节
;			-> 8字节内存基地址 + 8字节内存长度 + 4字节内存类型 
;==================================
	mov ecx,20 ;必须是20字节
	mov edx,0534d4150h ; edx必须为0534d4150h
	int 15h ; 执行15号中断
	jc .1  ;如果flags寄存器的C位置1 ,则跳转到  出错逻辑

	add edi,20 ; 更新下一次输出 结果的地址
	cmp edi,E80MAP_ADR+0x1000 ;比较 下一次的地址 是否是 
	jg .1

	inc esi

	cmp ebx,0
	jne loop ; 循环获取 e820map结构

	jmp .2

.1:
	mov esi,0 ; 出错处理, e820map结构数组 元素个数 为 0 

.2:
	mov dword[E80MAP_NR],esi ; e820map 结构数组元素个数
	pop ss
	pop es
	pop ds
	ret
_read:
	push ds
	push es
	push ss
	xor eax,eax
	mov ah,0x42
	mov dl,0x80
	mov si,RWHDPACK_ADR
	int 0x13
	jc  .err
	pop ss
	pop es
	pop ds
	ret
.err:
	mov ax,int131errmsg
	call DispStr
	jmp $
	pop ss
	pop es
	pop ds
	ret

;====================================================
;功能: 0x00 返回控制器信息 
;	依赖: 在实模式下经过 0x10 中断调用。
;	输入
;		ax= 0x4f00 必须
;		ES:DI =     指向存放VBEINFOBLOCK结构体的缓冲区指针
;	输出:
;		ax =  vbe返回状态
;	返回: 返回一个VBEINFOBLACK结构体 
;		暨 vbeinfo_t
;====================================================

_getvbemode:
        push es
        push ax
        push di
        mov di,VBEINFO_ADR
        mov ax,0
        mov es,ax
        mov ax,0x4f00
        int 0x10
        cmp ax,0x004f
        jz .ok
        mov ax,getvbmodeerrmsg
        call DispStr
        jmp $
 .ok:
        pop di
        pop ax
        pop es
        ret

;==============================================
;	功能: Return VBE mode information（返回 具体的VBE 模式信息)
;		输入: 
;			AX = 0x4F01 
;			CX = Mode number（模式号）  ==> 这里模式号采用0x118
;			ES : DI = Return VBE mode information（指向 ModeInfoBlock 结构体的缓冲区指针）
;					==> 暨 vbeminfo_t 
;==============================================
_getvbeonemodeinfo:
        push es
        push ax
        push di
        push cx
        mov di,VBEMINFO_ADR
        mov ax,0
        mov es,ax
        mov cx,0x118
        mov ax,0x4f01
        int 0x10
        cmp ax,0x004f
        jz .ok
        mov ax,getvbmodinfoeerrmsg
        call DispStr
        jmp $
 .ok:
        pop cx
        pop di
        pop ax
        pop es
        ret
;================================================
;功能: 设置VBE模式,暨按照输入的编号设置图像模式
;	 AX = 0x4F02
;	 BX = Desired Mode to set（须要设置的模式）
;	      D0 - D8  = Mode number （模式号）
;	      D9 - D10 = Reserved (must be 0)（保留，必须为 0）
;	      D11 = 0  Use current default refresh rate（使用当前缺省刷新率）
;	          = 1  Use user specified CRTC values for refresh rate（使用用户指定的 CRTC
;	                                                                         值为刷新率）
;	      D12 - 13 Reserved for VBE/AF (must be 0)（为 VBE/AF 保留，必须为 0）
;	      D14 = 0  Use windowed frame buffer model（使用窗口帧缓冲区模式）
;	          = 1  Use linear/flat frame buffer model（使用线性/平坦帧缓冲区模式）
;	      D15 = 0  Clear display memory（清除显示内存）
;	          = 1  Don't clear display memory（不清除显示内存）
;	 ES:DI = Pointer to CRTCInfoBlock structure（指向 CRTCInfoBlock 结构体的缓冲区指针）
;================================================

_setvbemode:
        push ax
        push bx
        mov bx,0x4118
        mov ax,0x4f02
        int 0x10
        cmp ax,0x004f
        jz .ok
        mov ax,setvbmodeerrmsg
        call DispStr
        jmp $
 .ok:
        pop bx
        pop ax
        ret
disable_nmi:
	push ax
	in al, 0x70     ; port 0x70NMI_EN_PORT
	or al, 0x80	; disable all NMI source
	out 0x70,al	; NMI_EN_PORT
	pop ax
	ret

func_table: ; 函数表
	dw _getmmap ;获取内存布局视图的函数
	dw _read ;读取硬盘的函数
        dw _getvbemode ; 获取显卡VBE模式
        dw _getvbeonemodeinfo ;获取显卡VBE 模式的数据
        dw _setvbemode ;设置显卡VBE模式


int131errmsg: db     "int 13 read hdsk  error"
        db 0
getvbmodeerrmsg: db     "get vbemode err"
        db 0
getvbmodinfoeerrmsg: db     "get vbemodeinfo err"
                db 0
setvbmodeerrmsg: db     "set vbemode err"
        db 0
