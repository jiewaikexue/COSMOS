

/* 时间: 2022年11月3日16:19:19
// MDC_ENDGIC和MDC_RVGIC这两个校验数是怎么来的呢？是eki规范还是自定义的？
// 自己定义的
// IMGFILE_PHYADR 0x4000000 这个值是怎么来的？
// 是 自定义的

#define MDC_ENDGIC 0xaaffaaffaaffaaff
#define MDC_RVGIC 0xffaaffaaffaaffaa
#define REALDRV_PHYADR 0x1000
#define IMGFILE_PHYADR 0x4000000 // 这个地址 使我们设置的_strat地址
// 所以 将映像文件加载到内存之后 就可以直接执行了
#define IMGKRNL_PHYADR 0x2000000 //  二级引导加载器装载的位置
#define LDRFILEADR IMGFILE_PHYADR
#define MLOSDSC_OFF (0x1000)
//  eki加载到了0x400000的地方，这个文件前4kb的空间是引导程序 
//  所以eki文件的管理头的数据就在0x201000的地址上开始 
#define MRDDSC_ADR (mlosrddsc_t*)(LDRFILEADR+0x1000)

*/
#include "cmctl.h"

void inithead_entry()
{
    init_curs();// 初始化光标
    close_curs(); // 关闭光标
    clear_screen(VGADP_DFVL); //清屏


    // 两个bin文件写入到特定的内存中
    // initldrkrl.bin:
    // initldrksva.bin:
    write_realintsvefile();  
    write_ldrkrlfile();

    return;
}

// 将initldrsve.bin文件写入到特定内存中
void write_realintsvefile()
{

    fhdsc_t *fhdscstart = find_file("initldrsve.bin");
    if (fhdscstart == NULL)
    {
        error("not file initldrsve.bin");
    }
    m2mcopy((void *)((u32_t)(fhdscstart->fhd_intsfsoff) + LDRFILEADR),
            (void *)REALDRV_PHYADR, (sint_t)fhdscstart->fhd_frealsz);
    return;
}

/*
* 正如其名，find_file 函数负责扫描映像文件中的文件头描述符，
* 对比其中的文件名，然后返回对应的文件头描述符的地址，
* 这样就可以得到文件在映像文件中的位置和大小了。
*/ 
fhdsc_t *find_file(char_t *fname)
{
    // #define IMGFILE_PHYADR 0x4000000
    // #define LDRFILEADR IMGFILE_PHYADR
    // #define MRDDSC_ADR (mlosrddsc_t*)(LDRFILEADR+0x1000)
    // 暨 MRDDSC_ADR : 是 (mlosrddsc_t *) (0x4000000 + 0x1000) 
    // 0x4000000: 地址装载的是 :
    // 0x4000000 + 0x1000 


    // eki文件加载到 _start指向的位置: 0x4000000
    // eki文件前4kb 是引导程序 真正的数据 从0x4010000 开始
    mlosrddsc_t *mrddadrs = MRDDSC_ADR;


    // #define MDC_ENDGIC 0xaaffaaffaaffaaff
    // #define MDC_RVGIC  0xffaaffaaffaaffaa

    // MDC_ENDGIC && MDC_RVGIC :两个 自定义校验数字
    if (mrddadrs->mdc_endgic != MDC_ENDGIC ||
        mrddadrs->mdc_rv != MDC_RVGIC ||
        mrddadrs->mdc_fhdnr < 2 ||
        mrddadrs->mdc_filnr < 2)
    {
        error("no mrddsc");
    }

    s64_t rethn = -1;
    fhdsc_t *fhdscstart = (fhdsc_t *)((u32_t)(mrddadrs->mdc_fhdbk_s) + LDRFILEADR);

    //依次扫描映像文件中国 文件头描述符
    for (u64_t i = 0; i < mrddadrs->mdc_fhdnr; i++)
    {
        if (strcmpl(fname, fhdscstart[i].fhd_name) == 0)
        {
            rethn = (s64_t)i;
            goto ok_l;
        }
    }
    rethn = -1;
ok_l:
    if (rethn < 0)
    {
        error("not find file");
    }
    return &fhdscstart[rethn];
}


// init +loader+kernel == initldrkrl
// init + loader + save
// initldrkrl.bin: 二级引导器的核心文件 : 装载到 0x2000000
// initldrsve.bin:  暂时还不知道是干啥的 ,装载到 0x1000
// initldrkrl.bin = initldrkrl.elf = ( ldrkrl32.o ldrkrlentry.o fs.o chkcpmm.o graph.o bstartparm.o vgastr.o )
//写initldrkrl.bin文件到特定的真实物理内存中 : 0x2000000
void write_ldrkrlfile()
{
    fhdsc_t *fhdscstart = find_file("initldrkrl.bin");
    if (fhdscstart == NULL)
    {
        error("not file initldrkrl.bin");
    }
    m2mcopy((void *)((u32_t)(fhdscstart->fhd_intsfsoff) + LDRFILEADR),
            (void *)ILDRKRL_PHYADR, (sint_t)fhdscstart->fhd_frealsz);
    
    return;
}


void error(char_t *estr)
{
    kprint("INITLDR DIE ERROR:%s\n", estr);
    for (;;)
        ;
    return;
}

int strcmpl(const char *a, const char *b)
{

    while (*b && *a && (*b == *a))
    {

        b++;
        a++;
    }

    return *b - *a;
}
