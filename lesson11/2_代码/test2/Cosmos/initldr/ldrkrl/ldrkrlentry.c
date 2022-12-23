

#include "cmctl.h"

extern idtr_t IDT_PTR;


/*
 ldrkrl_entry() 函数:
    在 initldr32.asm 文件（配套代码库中对应 ldrkrl32.asm）中被调用，从那条 call ldrkrl_entry 指令开始进入了 ldrkrl_entry() 函数
*/
void ldrkrl_entry()
{
    init_curs(); //初始化光标
    close_curs(); //关闭光标
    clear_screen(VGADP_DFVL); //清屏
    init_bstartparm(); // 搜集机器信息!
    return;
}



void kerror(char_t* kestr)
{
    kprint("INITKLDR DIE ERROR:%s\n",kestr);
    for(;;);
    return;
}

#pragma GCC push_options
#pragma GCC optimize ("O0")
// 主动死机
void die(u32_t dt)
{

    u32_t dttt=dt,dtt=dt;
    if(dt==0)
    {
        for(;;);
    }

    for(u32_t i=0;i<dt;i++)
    {
        for(u32_t j=0;j<dtt;j++)
        {
            for(u32_t k=0;k<dttt;k++)
            {
                ;
            }
        }
    }



    return;
}
#pragma GCC pop_options
