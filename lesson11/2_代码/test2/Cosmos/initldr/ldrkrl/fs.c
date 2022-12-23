#include "cmctl.h"

void fs_entry()
{
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

/*
    功能: 找到指定名字的文件信息,返回 改文件的文件描述符
    该函数请仔细对比 inithead.c 内部 find_file函数
    只能说是一模一样
*/
fhdsc_t *get_fileinfo(char_t *fname, machbstart_t *mbsp)
{

    mlosrddsc_t *mrddadrs = (mlosrddsc_t *)((u32_t)(mbsp->mb_imgpadr + MLOSDSC_OFF));
    if (mrddadrs->mdc_endgic != MDC_ENDGIC ||
        mrddadrs->mdc_rv != MDC_RVGIC ||
        mrddadrs->mdc_fhdnr < 2 ||
        mrddadrs->mdc_filnr < 2)
    {
        kerror("no mrddsc");
    }

    s64_t rethn = -1;
    fhdsc_t *fhdscstart = (fhdsc_t *)((u32_t)(mrddadrs->mdc_fhdbk_s) + ((u32_t)(mbsp->mb_imgpadr)));

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
        kerror("not find file");
    }
    return &fhdscstart[rethn];
}

/*
    主要负责判断一个地址空间是否和内存中存放的内容有冲突。
    因为我们的内存中已经放置了机器信息结构、内存视图结构数组、二级引导器、内核映像文件，
    所以在处理内存空间时不能和内存中已经存在的他们冲突，
    否则就要覆盖他们的数据。
    0x8f000～（0x8f000+0x1001），正是我们的内核栈空间，我们需要检测它是否和其它空间有冲突。
*/
int move_krlimg(machbstart_t *mbsp, u64_t cpyadr, u64_t cpysz)
{

    if (0xffffffff <= (cpyadr + cpysz) || 1 > cpysz)
    {
        return 0;
    }
    void *toadr = (void *)((u32_t)(P4K_ALIGN(cpyadr + cpysz)));
    sint_t tosz = (sint_t)mbsp->mb_imgsz;
    if (0 != adrzone_is_ok(mbsp->mb_imgpadr, mbsp->mb_imgsz, cpyadr, cpysz))
    {
        if (NULL == chk_memsize((e820map_t *)((u32_t)(mbsp->mb_e820padr)), (u32_t)mbsp->mb_e820nr, (u64_t)((u32_t)toadr), (u64_t)tosz))
        {
            return -1;
        }
        m2mcopy((void *)((u32_t)mbsp->mb_imgpadr), toadr, tosz);
        mbsp->mb_imgpadr = (u64_t)((u32_t)toadr);
        return 1;
    }
    return 2;
}

/*
    放置 内核文件
    因为我们的内核已经编译成了 一个独立的二进制程序 ，和其它文件一起被 打包 到 映像文件 中了。
    所以我们必须要从映像中把它解包出来，将其放在特定的物理内存空间中才可以，
    放置字库文件和放置内核文件的原理一样，所以我们来一起实现。
*/
void init_krlfile(machbstart_t *mbsp)
{
    //在映像中查找相应的文件，并复制到对应的地址，并返回文件的大小，这里是查找kernel.bin文件
    u64_t sz = r_file_to_padr(mbsp, IMGKRNL_PHYADR, "kernel.bin");
    if (0 == sz)
    {
        kerror("r_file_to_padr err");
    }
    //放置完成后 更新机器信息 结构 中的数据 
    mbsp->mb_krlimgpadr = IMGKRNL_PHYADR; // 内核映像文件的地址
    mbsp->mb_krlsz = sz; // 内核大小
    ////mbsp->mb_nextwtpadr 始终要指向 下一段空闲内存的首地址
    mbsp->mb_nextwtpadr = P4K_ALIGN(mbsp->mb_krlimgpadr + mbsp->mb_krlsz);
    mbsp->mb_kalldendpadr = mbsp->mb_krlimgpadr + mbsp->mb_krlsz;
    return;
}

/* 
    放置 内核字体库文件

*/
void init_defutfont(machbstart_t *mbsp)
{
    u64_t sz = 0;
    // 获取下一段 空闲内存空间的首地址
    u32_t dfadr = (u32_t)mbsp->mb_nextwtpadr;
    //在映像中查找相应的文件，并复制到对应的地址，并返回文件的大小，这里是查找font.fnt文件
    sz = r_file_to_padr(mbsp, dfadr, "font.fnt");
    if (0 == sz)
    {
        kerror("r_file_to_padr err");
    }
    // 字体文件的首地址
    mbsp->mb_bfontpadr = (u64_t)(dfadr);
    // 字体库文件的大小
    mbsp->mb_bfontsz = sz;
    // 更新 机器信息结构中下一段空闲内存的地址
    mbsp->mb_nextwtpadr = P4K_ALIGN((u32_t)(dfadr) + sz);
    mbsp->mb_kalldendpadr = mbsp->mb_bfontpadr + mbsp->mb_bfontsz;
    return;
}

/*
    功能: 获取目标文件 在整个映像文件 中的 地址 and 大小
*/
void get_file_rpadrandsz(char_t *fname, machbstart_t *mbsp, u32_t *retadr, u32_t *retsz)
{
    u64_t padr = 0, fsz = 0;
    if (NULL == fname || NULL == mbsp)
    {
        *retadr = 0;
        return;
    }
    fhdsc_t *fhdsc = get_fileinfo(fname, mbsp);
    if (fhdsc == NULL)
    {
        *retadr = 0;
        return;
    }
    /*
        都是一个大的eki文件, 基地址+ 偏移量的思想 去的 目标文件的其实地址
    */
    padr = fhdsc->fhd_intsfsoff + mbsp->mb_imgpadr;
    if (padr > 0xffffffff)
    {
        *retadr = 0;
        return;
    }
    fsz = (u32_t)fhdsc->fhd_frealsz;
    if (fsz > 0xffffffff)
    {
        *retadr = 0;
        return;
    }
    *retadr = (u32_t)padr;
    *retsz = (u32_t)fsz;
    return;
}

u64_t get_filesz(char_t *filenm, machbstart_t *mbsp)
{
    if (filenm == NULL || mbsp == NULL)
    {
        return 0;
    }
    fhdsc_t *fhdscstart = get_fileinfo(filenm, mbsp);
    if (fhdscstart == NULL)
    {
        return 0;
    }
    return fhdscstart->fhd_frealsz;
}

u64_t get_wt_imgfilesz(machbstart_t *mbsp)
{
    u64_t retsz = LDRFILEADR;
    mlosrddsc_t *mrddadrs = MRDDSC_ADR;
    if (mrddadrs->mdc_endgic != MDC_ENDGIC ||
        mrddadrs->mdc_rv != MDC_RVGIC ||
        mrddadrs->mdc_fhdnr < 2 ||
        mrddadrs->mdc_filnr < 2)
    {
        return 0;
    }
    if (mrddadrs->mdc_filbk_e < 0x4000)
    {
        return 0;
    }
    retsz += mrddadrs->mdc_filbk_e;
    retsz -= LDRFILEADR;
    mbsp->mb_imgpadr = LDRFILEADR;
    mbsp->mb_imgsz = retsz;
    return retsz;
}

/*
    功能: 在映像中查找 kernel.bin 和 font.fnt 文件，并复制到对应的空闲内存空间中。
*/
u64_t r_file_to_padr(machbstart_t *mbsp, u32_t f2adr, char_t *fnm)
{
    if (NULL == f2adr || NULL == fnm || NULL == mbsp)
    {
        return 0;
    }
    u32_t fpadr = 0, sz = 0;
    get_file_rpadrandsz(fnm, mbsp, &fpadr, &sz);
    if (0 == fpadr || 0 == sz)
    {
        return 0;
    }
    if (NULL == chk_memsize((e820map_t *)((u32_t)mbsp->mb_e820padr), (u32_t)(mbsp->mb_e820nr), f2adr, sz))
    {
        return 0;
    }
    if (0 != chkadr_is_ok(mbsp, f2adr, sz))
    {
        return 0;
    }
    m2mcopy((void *)fpadr, (void *)f2adr, (sint_t)sz);
    return sz;
}

u64_t ret_imgfilesz()
{
    u64_t retsz = LDRFILEADR;
    mlosrddsc_t *mrddadrs = MRDDSC_ADR;
    if (mrddadrs->mdc_endgic != MDC_ENDGIC ||
        mrddadrs->mdc_rv != MDC_RVGIC ||
        mrddadrs->mdc_fhdnr < 2 ||
        mrddadrs->mdc_filnr < 2)
    {
        kerror("no mrddsc");
    }
    if (mrddadrs->mdc_filbk_e < 0x4000)
    {
        kerror("imgfile error");
    }
    retsz += mrddadrs->mdc_filbk_e;
    retsz -= LDRFILEADR;
    return retsz;
}
