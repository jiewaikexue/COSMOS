
#include "cmctl.h"

unsigned int acpi_get_bios_ebda()
{
    unsigned int address = *(unsigned short *)0x40E;
    address <<= 4;
    return address;
}

int acpi_checksum(unsigned char *ap, s32_t len)
{
    int sum = 0;
    while (len--)
    {
        sum += *ap++;
    }
    return sum & 0xFF;
}

mrsdp_t *acpi_rsdp_isok(mrsdp_t *rdp)
{
    if (rdp->rp_len == 0 || rdp->rp_revn == 0)
    {
        return NULL;
    }
    if (0 == acpi_checksum((unsigned char *)rdp, (s32_t)rdp->rp_len))
    {

        return rdp;
    }

    return NULL;
}

mrsdp_t *findacpi_rsdp_core(void *findstart, u32_t findlen)
{
    if (NULL == findstart || 1024 > findlen)
    {
        return NULL;
    }

    u8_t *tmpdp = (u8_t *)findstart;

    mrsdp_t *retdrp = NULL;
    for (u64_t i = 0; i <= findlen; i++)
    {
        if (('R' == tmpdp[i]) && ('S' == tmpdp[i + 1]) && ('D' == tmpdp[i + 2]) && (' ' == tmpdp[i + 3]) &&
            ('P' == tmpdp[i + 4]) && ('T' == tmpdp[i + 5]) && ('R' == tmpdp[i + 6]) && (' ' == tmpdp[i + 7]))
        {
            retdrp = acpi_rsdp_isok((mrsdp_t *)(&tmpdp[i]));
            if (NULL != retdrp)
            {
                return retdrp;
            }
        }
    }
    return NULL;
}

mrsdp_t *find_acpi_rsdp()
{

    void *fndp = (void *)acpi_get_bios_ebda();
    mrsdp_t *rdp = findacpi_rsdp_core(fndp, 1024);
    if (NULL != rdp)
    {
        return rdp;
    }

    fndp = (void *)(0xe0000);
    rdp = findacpi_rsdp_core(fndp, (0xfffff - 0xe0000));
    if (NULL != rdp)
    {
        return rdp;
    }
    return NULL;
}

void init_acpi(machbstart_t *mbsp)
{
    mrsdp_t *rdp = NULL;
    rdp = find_acpi_rsdp();
    if (NULL == rdp)
    {
        kerror("Your computer is not support ACPI!!");
    }
    m2mcopy(rdp, &mbsp->mb_mrsdp, (sint_t)((sizeof(mrsdp_t))));
    if (acpi_rsdp_isok(&mbsp->mb_mrsdp) == NULL)
    {
        kerror("Your computer is not support ACPI!!");
    }
    return;
}

/*功能: 利用汇编的15号中断 暨mmap函数 来获取内存布局信息
*/
void init_mem(machbstart_t *mbsp)
{
    e820map_t *retemp; // e820map 结构首地址
    u32_t retemnr = 0; //e820map 结构 内部数组元素个数
    mbsp->mb_ebdaphyadr = acpi_get_bios_ebda();
    mmap(&retemp, &retemnr);
    if (retemnr == 0)
    {
        kerror("no e820map\n");
    }
    // 根据e820map_t结构数据检查内存大小
    if (chk_memsize(retemp, retemnr, 0x100000, 0x8000000) == NULL)
    {
        kerror("Your computer is low on memory, the memory cannot be less than 128MB!");
    }

    
    mbsp->mb_e820padr = (u64_t)((u32_t)(retemp));//把e820map_t结构数组的首地址传给mbsp->mb_e820padr
    mbsp->mb_e820nr = (u64_t)retemnr;//把e820map_t结构数组元素个数传给mbsp->mb_e820nr 
    mbsp->mb_e820sz = retemnr * (sizeof(e820map_t));//把e820map_t结构数组大小传给mbsp->mb_e820sz
    mbsp->mb_memsz = get_memsize(retemp, retemnr);//根据e820map_t结构数据计算内存大小。
    init_acpi(mbsp);
    return;
}
/*功能:  利用CPUID指令 检查CPU是否支持64位长模式
    1. 调用 chk_cpuid :检查是否支持CPUID指令
    2. 调用 chk_cpu_longmode : 在支持CPUID的基础上,检查是否支持长模式
参数: 
    mbsp:二级引导器获取的信息存放的结构体

*/
void init_chkcpu(machbstart_t *mbsp)
{
    if (!chk_cpuid())
    {
        kerror("Your CPU is not support CPUID sys is die!");
        
        //#define CLI_HALT() __asm__ __volatile__("cli; hlt": : :"memory")
        CLI_HALT();
    }

    if (!chk_cpu_longmode())
    {
        kerror("Your CPU is not support 64bits mode sys is die!");
        CLI_HALT();
    }
    mbsp->mb_cpumode = 0x40; //cpu 模式 64位
    return;
}
/*初始化内核栈
*/
void init_krlinitstack(machbstart_t *mbsp)
{
    if (1 > move_krlimg(mbsp, (u64_t)(0x8f000), 0x1001))
    {
        kerror("iks_moveimg err");
    }
    mbsp->mb_krlinitstack = IKSTACK_PHYADR; //栈顶地址
    mbsp->mb_krlitstacksz = IKSTACK_SIZE; // 栈大小 24kb
    return;
}

/*功能: 建立MMU页表数据
    init_board_start_pages:  
    
    #define KINITPAGE_PHYADR 0x1000000 
    利用二级引导器, 建立 MMU页表数据
    目的: --> 在内核加载运行之初 假期CPU长模式时, 就需要使用已经建立好的 MMU页表结构
        --> 长模式下 MMU页表 分页粒度 2MB

    整个MMU页表：
        起始地址：0x100 0000
        顶级目录页：0x100 0000 ~ 0x100 FFFF，这里只有第0个64位存放了有效数据，其页目录指针页的地址，剩余511个64位没用上。
        页目录指针页：0x100 1000 ~ 0x100 1FFF，这里只有第0~15个64位存放了有效数据，每个64位中保存了一个页目录指针，通过他可以找到下一级的页目录地址；剩余的496个64位数据没用上。
        页目录页：0x100 2000 ~ (0x100 2000 + 0x1000*16 - 1)，共16页，每页里面512项，每项保存着2MB物理内存的地址。
    
*/
void init_bstartpages(machbstart_t *mbsp)
{
    // 页 顶级目录
    u64_t *p = (u64_t *)(KINITPAGE_PHYADR); // 逻辑地址16MB地址处
    // 页 目录指针
    u64_t *pdpte = (u64_t *)(KINITPAGE_PHYADR + 0x1000);//页目录指针
    // 页 目录
    u64_t *pde = (u64_t *)(KINITPAGE_PHYADR + 0x2000);
    
    // 物理地址从0 开始
    u64_t adr = 0;

    if (1 > move_krlimg(mbsp, (u64_t)(KINITPAGE_PHYADR), (0x1000 * 16 + 0x2000)))
    {
        kerror("move_krlimg err");
    }

    // 将顶级 页目录, 页目录指针 清0
    for (uint_t mi = 0; mi < PGENTY_SIZE; mi++)
    {
        p[mi] = 0;
        pdpte[mi] = 0;
    }
    /*
    映射:
        有16个项，每个项有512个大页，每个大页2MB。
        外层循环 控制 项数
        内层循环 控制 每一个 页表顶级目录 の 内部 的页
        暨 16个 页表顶级目录项 ,每一个控制1G 空间
          每1G空间 内部划分成2MB 的512个小页
          这样总共最大就可以支持16G 内存了
    */
    for (uint_t pdei = 0; pdei < 16; pdei++)
    {
        pdpte[pdei] = (u64_t)((u32_t)pde | KPDPTE_RW | KPDPTE_P);

        /*
            物理地址每次增加 2MB，这是内层for循环控制的，
            每执行一次外层循环就要执行 512 次内层循环。
        */
        for (uint_t pdeii = 0; pdeii < PGENTY_SIZE; pdeii++)
        {
            // 大页 KPDE_PS 2MB ,可读可写 KPDE_RW 存在KPDE_P 
            pde[pdeii] = 0 | adr | KPDE_PS | KPDE_RW | KPDE_P;
            adr += 0x200000;
        }
        pde = (u64_t *)((u32_t)pde + 0x1000);
    }
    
    /*
        让 "顶级页目录" 中 第0项: p[0]: 虚拟地址：0～0x400000000
        和 CPU 目的地的真实地址 ：0xffff800000000000～0xffff800400000000 和 虚拟地址：0～0x400000000，
                ====>  访问到同一个物理地址空间 0～0x400000000
            |===============================================
            + 计算 : ((KRNL_VIRTUAL_ADDRESS_START) >> KPML4_SHIFT & 0x1ff = 256
            +
            +     暨 : 页表第 0 项 and 第256 项 映射相同
            ================================================== 

            关于为什么要如此映射 : 根本 目的: 内核在启动初期 虚拟地址 和物理地址保持相同
                            =======> 因为你是在内核态!
        ==============================================================
        + 这就是传说中的平坦映射。
        + 是因为开启了mmu之后，cpu看到的都认为是虚拟地址，
        + 但是这个时候 "有些指令依赖符号跳转时还是按照 物理地址 跳转的，
        + 所以也需要 物理地址跳转的地址 还得是 物理地址 ，
        + 只好把虚拟地址和物理地址搞成相同的。
        ==============================================================
        + 前者是为了做平坦映射便于OS的实现，
        + 后者则是真实地把内核的虚拟地址映射到物理地址，
        + 因此也可以看出不同进程进入到内核态之后，
        + 虚拟地址和物理地址的映射是一致的，
        + 因此不同进程共享同一个内核堆
        ==============================================================
        + 官方: 在内核启动初期, 虚拟地址 和 物理地址 要保持相同
        +     暨 内核态 的地址是虚拟地址是固定的
        ==============================================================

    */
    p[((KRNL_VIRTUAL_ADDRESS_START) >> KPML4_SHIFT) & 0x1ff] = (u64_t)((u32_t)pdpte | KPML4_RW | KPML4_P);
    p[0] = (u64_t)((u32_t)pdpte | KPML4_RW | KPML4_P);
    
    // 把业表首地址 保存在 机器信息结构中
    mbsp->mb_pml4padr = (u64_t)(KINITPAGE_PHYADR);
    mbsp->mb_subpageslen = (u64_t)(0x1000 * 16 + 0x2000);
    mbsp->mb_kpmapphymemsz = (u64_t)(0x400000000);
    return;
}

void init_meme820(machbstart_t *mbsp)
{
    e820map_t *semp = (e820map_t *)((u32_t)(mbsp->mb_e820padr));
    u64_t senr = mbsp->mb_e820nr;
    e820map_t *demp = (e820map_t *)((u32_t)(mbsp->mb_nextwtpadr));
    if (1 > move_krlimg(mbsp, (u64_t)((u32_t)demp), (senr * (sizeof(e820map_t)))))
    {
        kerror("move_krlimg err");
    }

    m2mcopy(semp, demp, (sint_t)(senr * (sizeof(e820map_t))));
    mbsp->mb_e820padr = (u64_t)((u32_t)(demp));
    mbsp->mb_e820sz = senr * (sizeof(e820map_t));
    mbsp->mb_nextwtpadr = P4K_ALIGN((u32_t)(demp) + (u32_t)(senr * (sizeof(e820map_t))));
    mbsp->mb_kalldendpadr = mbsp->mb_e820padr + mbsp->mb_e820sz;
    return;
}

/*功能: init_mem 内部调用 e820map的起始地址和内部元素个数 写入到某个地址
    retemp:e820map结构首地址
    retemnr: e820map结构内部元素个数
*/
void mmap(e820map_t **retemp, u32_t *retemnr)
{
    //调用 asm文件中的函数
    realadr_call_entry(RLINTNR(0), 0, 0);
    /*
        inc esi     
        mov dword[E80MAP_NR],esi ; e820map 结构数组元素个数
    */
    *retemnr = *((u32_t *)(E80MAP_NR));
    //汇编获取:e829mao结构起始地址: mov dword[E80MAP_ADRADR],E80MAP_ADR
    *retemp = (e820map_t *)(*((u32_t *)(E80MAP_ADRADR)));
    return;
}

e820map_t *chk_memsize(e820map_t *e8p, u32_t enr, u64_t sadr, u64_t size)
{
    u64_t len = sadr + size;
    if (enr == 0 || e8p == NULL)
    {
        return NULL;
    }
    for (u32_t i = 0; i < enr; i++)
    {
        if (e8p[i].type == RAM_USABLE)
        {
            if ((sadr >= e8p[i].saddr) && (len < (e8p[i].saddr + e8p[i].lsize)))
            {
                return &e8p[i];
            }
        }
    }
    return NULL;
}

u64_t get_memsize(e820map_t *e8p, u32_t enr)
{
    u64_t len = 0;
    if (enr == 0 || e8p == NULL)
    {
        return 0;
    }
    for (u32_t i = 0; i < enr; i++)
    {
        if (e8p[i].type == RAM_USABLE)
        {
            len += e8p[i].lsize;
        }
    }
    return len;
}

/*功能:   通过改写Eflags寄存器的第21位，观察其位的变化判断是否支持CPUID
    --->  EFLAGS寄存器的第21位用于指示是否CPUID能够被使用。如果软件能够对它进行设置或者清除，那么当前系统是可利用的
    ---> 
        EFLAGS寄存器我们不能够直接访问，
        所以通过PUSHFD/POPFD来完成相应的修改动作。
        程序的基本思路是先将EFLAGS存放在EAX/EBX中，
        在修改它的第21位后通过POPFD来完成更新工作。
        在此之后，通过再次获取它的当前值（存放在EAX中）与修改前之值（存放在EBX中）进行比较，
        若发现一致，则表面当前系统不支持CPUID指令，反之亦然。
        原文链接：https://blog.csdn.net/hello_wyq/article/details/3393815
*/
int chk_cpuid()
{
    int rets = 0;
    __asm__ __volatile__(
        "pushfl \n\t"
        "popl %%eax \n\t"
        "movl %%eax,%%ebx \n\t"
        "xorl $0x0200000,%%eax \n\t"
        "pushl %%eax \n\t"
        "popfl \n\t"
        "pushfl \n\t"
        "popl %%eax \n\t"
        "xorl %%ebx,%%eax \n\t"
        "jz 1f \n\t"
        "movl $1,%0 \n\t"
        "jmp 2f \n\t"
        "1: movl $0,%0 \n\t"
        "2: \n\t"
        : "=c"(rets)
        :
        :);
    return rets;
}

/*功能: 利用CPUID指令,检查cpu是否支持长模式:
    
    ===>
        在汇编语言中，CPUID指令不使用参数，
        因为CPUID隐式使用EAX寄存器来确定返回信息的主类别。
        在英特尔最新的术语中，这被称为CPUID。
        CPUID的调用应该以EAX = 0开始，这将在EAX寄存器中返回CPU支持的最高EAX调用参数（leaf）。
        eg: 0x80000000
*/
int chk_cpu_longmode()
{
    int rets = 0;
    __asm__ __volatile__(
        "movl $0x80000000,%%eax \n\t"
        "cpuid \n\t" //把eax寄存器中放入 0x800000000,并调用CPUID指令
                //  100000000000000000000000000000000000 : 如果可以 支持64位的话 就是 100000000000000000000000000000000001
        "cmpl $0x80000001,%%eax \n\t" //不为0x80000001,则不支持0x80000001号功能
        "setnb %%al \n\t"
        "jb 1f \n\t"
        "movl $0x80000001,%%eax \n\t"
        "cpuid \n\t" //把eax中放入0x800000001调用CPUID指令，检查edx中的返回数据
        "bt $29,%%edx  \n\t" //长模式 支持位 是否为1
        "setcb %%al \n\t"
        "1: \n\t"
        "movzx %%al,%%eax \n\t"
        : "=a"(rets)
        :
        :);
    return rets;
}

void init_chkmm()
{

    e820map_t *map = (e820map_t *)EMAP_PTR;
    u16_t *map_nr = (u16_t *)EMAP_NR_PTR;
    u64_t mmsz = 0;

    for (int j = 0; j < (*map_nr); j++)
    {
        if (map->type == RAM_USABLE)
        {
            mmsz += map->lsize;
        }
        map++;
    }

    if (mmsz < BASE_MEM_SZ) //0x3F00000
    {
        kprint("Your computer is low on memory, the memory cannot be less than 64MB!");
        CLI_HALT();
    }

    if (!chk_cpuid())
    {
        kprint("Your CPU is not support CPUID sys is die!");
        CLI_HALT();
    }

    if (!chk_cpu_longmode())
    {
        kprint("Your CPU is not support 64bits mode sys is die!");
        CLI_HALT();
    }
    ldr_createpage_and_open();
    //for(;;);
    return;
}

void out_char(char *c)
{

    char *str = c, *p = (char *)0xb8000;

    while (*str)
    {
        *p = *str;
        p += 2;
        str++;
    }

    return;
}

void init_bstartpagesold(machbstart_t *mbsp)
{

    if (1 > move_krlimg(mbsp, (u64_t)(PML4T_BADR), 0x3000))
    {
        kerror("ip_moveimg err");
    }

    pt64_t *pml4p = (pt64_t *)PML4T_BADR, *pdptp = (pt64_t *)PDPTE_BADR, *pdep = (pt64_t *)PDE_BADR; 
    for (int pi = 0; pi < PG_SIZE; pi++)
    {
        pml4p[pi] = 0;
        pdptp[pi] = 0;

        pdep[pi] = 0;
    }

    pml4p[0] = 0 | PDPTE_BADR | PDT_S_RW | PDT_S_PNT;
    pdptp[0] = 0 | PDE_BADR | PDT_S_RW | PDT_S_PNT;
    pml4p[256] = 0 | PDPTE_BADR | PDT_S_RW | PDT_S_PNT;

    pt64_t tmpba = 0, tmpbd = 0 | PDT_S_SIZE | PDT_S_RW | PDT_S_PNT;

    for (int di = 0; di < PG_SIZE; di++)
    {
        pdep[di] = tmpbd;
        tmpba += 0x200000;
        tmpbd = tmpba | PDT_S_SIZE | PDT_S_RW | PDT_S_PNT;
    }
    mbsp->mb_pml4padr = (u64_t)((u32_t)pml4p);
    mbsp->mb_subpageslen = 0x3000;
    mbsp->mb_kpmapphymemsz = (0x200000 * 512);
    return;
}

void ldr_createpage_and_open()
{
    pt64_t *pml4p = (pt64_t *)PML4T_BADR, *pdptp = (pt64_t *)PDPTE_BADR, *pdep = (pt64_t *)PDE_BADR; 
    for (int pi = 0; pi < PG_SIZE; pi++)
    {
        pml4p[pi] = 0;
        pdptp[pi] = 0;
        pdep[pi] = 0;
    }

    pml4p[0] = 0 | PDPTE_BADR | PDT_S_RW | PDT_S_PNT;
    pdptp[0] = 0 | PDE_BADR | PDT_S_RW | PDT_S_PNT;
  
    pml4p[256] = 0 | PDPTE_BADR | PDT_S_RW | PDT_S_PNT;

    pt64_t tmpba = 0, tmpbd = 0 | PDT_S_SIZE | PDT_S_RW | PDT_S_PNT;

    for (int di = 0; di < PG_SIZE; di++)
    {
        pdep[di] = tmpbd;
        tmpba += 0x200000;
        tmpbd = tmpba | PDT_S_SIZE | PDT_S_RW | PDT_S_PNT;
    }

    return;
}
