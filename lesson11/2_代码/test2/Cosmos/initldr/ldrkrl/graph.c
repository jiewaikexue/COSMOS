#include "cmctl.h"

void write_pixcolor(machbstart_t *mbsp, u32_t x, u32_t y, pixl_t pix)
{

    u8_t *p24bas;
    if (mbsp->mb_ghparm.gh_onepixbits == 24)
    {
        u32_t p24adr = (x + (y * mbsp->mb_ghparm.gh_x)) * 3;
        p24bas = (u8_t *)(p24adr + mbsp->mb_ghparm.gh_framphyadr);
        p24bas[0] = (u8_t)(pix);
        p24bas[1] = (u8_t)(pix >> 8);
        p24bas[2] = (u8_t)(pix >> 16);
        return;
    }
    u32_t *phybas = (u32_t *)mbsp->mb_ghparm.gh_framphyadr;
    phybas[x + (y * mbsp->mb_ghparm.gh_x)] = pix;

    return;
}

void bmp_print(void *bmfile, machbstart_t *mbsp)
{
    if (NULL == bmfile)
    {
        return;
    }
    pixl_t pix = 0;
    bmdbgr_t *bpixp = NULL;
    bmfhead_t *bmphdp = (bmfhead_t *)bmfile;
    bitminfo_t *binfp = (bitminfo_t *)(bmphdp + 1);
    u32_t img = (u32_t)bmfile + bmphdp->bf_off;
    bpixp = (bmdbgr_t *)img;
    int l = 0;
    int k = 0;
    int ilinebc = (((binfp->bi_w * 24) + 31) >> 5) << 2;
    for (int y = 639; y >= 129; y--, l++)
    {
        k = 0;
        for (int x = 322; x < 662; x++)
        {
            pix = BGRA(bpixp[k].bmd_r, bpixp[k].bmd_g, bpixp[k].bmd_b);
            if (50 < l)
            {
                if (0x00ffffff <= pix || pix >= 0x00999999)
                {
                    pix = 0;
                }
            }

            write_pixcolor(mbsp, x, y, pix);
            k++;
        }
        bpixp = (bmdbgr_t *)(((int)bpixp) + ilinebc);
    }
    return;
}

/*功能: 显示内核logo
    在图格式的文件中，除了文件头的数据就是图形像素点的数据，
    只不过 24 位的位图每个像素占用 3 字节，并且位置是倒排的，
    即第一个像素的数据是在文件的最后，依次类推。
    我们只要依次将位图文件的数据，按照倒排次序写入显存中，这样就可以显示了
    我们需要把二级引导器的文件和 logo 文件打包成映像文件，然后放在虚拟硬盘中
*/
void logo(machbstart_t *mbsp)
{
    u32_t retadr = 0, sz = 0;
    //在映像文件中获取logo.bmp文件
    get_file_rpadrandsz("logo.bmp", mbsp, &retadr, &sz);
    if (0 == retadr)
    {
        kerror("if_getfilerpadrsz err");
    }
    //显示logo文件中的图像数据
    bmp_print((void *)retadr, mbsp);

    return;
}

/*功能:通过内部 多个函数调用,设置 图形模式
    1. 初始化图形数据结构
    2. 或缺VBE模式,通过BIOS中断获取
    3. 获取一个具体的VBE模式的信息,通过BIOS中断
    3. 设置VBE模式,通过BIOS中断

    =================================================
    + 在计算机加电启动时，计算机上显卡会自动进入文本模式，
    + 文本模式只能显示 ASCII 字符，不能显示汉字和图形，
    + 所以我们要让显卡切换到图形模式。
    =================================================
    +   VBE 是显卡的一个图形规范标准，
    +   它定义了显卡的几种图形模式，
    +   每个模式包括
    +           -> 屏幕分辨率，像素格式与大小，显存大小。
    +   调用 BIOS 10h 中断可以返回这些数据结构
    =================================================
    +   这里我们选择使用了 VBE 的 118h 模式，该模式下屏幕分辨率为 1024x768，
    +   显存大小是 16.8MB。
    +   显存开始地址一般为 0xe0000000。
    +   屏幕分辨率为 1024x768，即把屏幕分成 768 行，每行 1024 个像素点，
    +   但每个像素点占用显存的 32 位数据（4 字节，红、绿、蓝、透明各占 8 位）。
    +   我们只要往对应的显存地址写入相应的像素数据，屏幕对应的位置就能显示了。
    =================================================
    +                附：标准模式号列表
    +      文本模式
    +   分辨率    模式
    +   80x60     0x108
    +   132x132   0x109
    +   132x50    0x10b
    +   132x60    0x10c
    +================================================
    +       图形模式
    +   分辨率      4位     8位    15位    16位    32位（24位有效）
    +   320x200                 0x10d   0x10e   0x10f
    +   640x400           0x100
    +   640×480           0x101   0x110   0x111   0x112
    +   800x600      0x102   0x103   0x113   0x114   0x115
    +   1024×768     0x104   0x105   0x116   0x117   0x118
    +   1280×1024    0x106   0x107   0x119   0x11a   0x11b
    +   1600x1200          0x11c   0x11d   0x11e   0x11f
    +
    +   8 位：调色板模式
    +   15 位：R5 / G5 / B5（15 位的支持的是小型的，因此不能期望。）
    +   16 位：R5 / G6 / B5
    +   32 位：I8 / R8 / G8 / B8（I = ignore）
    +
    +   0x140之后：开发商自由可定义
*/
void init_graph(machbstart_t *mbsp)
{
    //初始化图形数据结构
    graph_t_init(&mbsp->mb_ghparm);
    init_bgadevice(mbsp);
    if (mbsp->mb_ghparm.gh_mode != BGAMODE)
    {
        //通过BIOS中断,获取VBE模式控制器信息
        get_vbemode(mbsp);
        //通过BIOS模式,来获取一个具体的VBE模式信息
        get_vbemodeinfo(mbsp);
        //设置VBE模式,通过BIOS中断
        set_vbemodeinfo();
    }

    init_kinitfvram(mbsp);
    //显示loho
    logo(mbsp);
    return;
}
/* 功能: 初始化 二级引导器搜索信息结构体中图像信息结构体
*/
void graph_t_init(graph_t *initp)
{
    memset(initp, 0, sizeof(graph_t));
    return;
}

void init_kinitfvram(machbstart_t *mbsp)
{
    mbsp->mb_fvrmphyadr = KINITFRVM_PHYADR;
    mbsp->mb_fvrmsz = KINITFRVM_SZ;
    memset((void *)KINITFRVM_PHYADR, 0, KINITFRVM_SZ);

    return;
}

u32_t vfartolineadr(u32_t vfar) 
{
    u32_t tmps = 0, sec = 0, off = 0;
    off = vfar & 0xffff;
    tmps = (vfar >> 16) & 0xffff;
    sec = tmps << 4;
    return (sec + off);
}

/*功能: 通过汇编调用 BIOS中断 来收集VBE控制器 的信息
*/
void get_vbemode(machbstart_t *mbsp)
{
    //调用 该函数 千万reaintsve.asm 根据 汇编函数表索引 执行 _getvbemode汇编函数 获取vbe模式系信息
    realadr_call_entry(RLINTNR(2), 0, 0);
    //vbe控制器信息指针
    vbeinfo_t *vbeinfoptr = (vbeinfo_t *)VBEINFO_ADR;
    u16_t *mnm;

    // 检查是否是VBE模式
    if (vbeinfoptr->vbesignature[0] != 'V' ||
        vbeinfoptr->vbesignature[1] != 'E' ||
        vbeinfoptr->vbesignature[2] != 'S' ||
        vbeinfoptr->vbesignature[3] != 'A')
    {
        kerror("vbe is not VESA");
    }
    kprint("vbe vbever:%x\n", vbeinfoptr->vbeversion);
    //检查 VBE模式版本
    if (vbeinfoptr->vbeversion < 0x0200)
    {
        kerror("vbe version not vbe3");
    }

    // 图形模式指针mnm
    if (vbeinfoptr->videomodeptr > 0xffff)
    {
        mnm = (u16_t *)vfartolineadr(vbeinfoptr->videomodeptr); //
    }
    else
    {
        mnm = (u16_t *)(vbeinfoptr->videomodeptr);
    }

    int bm = 0;
    // 找到第0x118号模式
    for (int i = 0; mnm[i] != 0xffff; i++)
    {
        if (mnm[i] == 0x118)
        {
            bm = 1;
        }
        if (i > 0x1000)
        {
            break;
        }
        //kprint("vbemode:%x  ",mnm[i]);
    }

    if (bm == 0)
    {
        kerror("getvbemode not 118");
    }
    // MBSP信息结构体重存放模式
    mbsp->mb_ghparm.gh_mode = VBEMODE;
    // 存放模式号
    mbsp->mb_ghparm.gh_vbemodenr = 0x118;
    // 
    mbsp->mb_ghparm.gh_vifphyadr = VBEINFO_ADR;
    m2mcopy(vbeinfoptr, &mbsp->mb_ghparm.gh_vbeinfo, sizeof(vbeinfo_t));
    return;
}

void bga_write_reg(u16_t index, u16_t data)
{
    out_u16(VBE_DISPI_IOPORT_INDEX, index);
    out_u16(VBE_DISPI_IOPORT_DATA, data);
    return;
}

u16_t bga_read_reg(u16_t index)
{
    out_u16(VBE_DISPI_IOPORT_INDEX, index);
    return in_u16(VBE_DISPI_IOPORT_DATA);
}

u32_t get_bgadevice()
{
    u16_t bgaid = bga_read_reg(VBE_DISPI_INDEX_ID);
    if (BGA_DEV_ID0 <= bgaid && BGA_DEV_ID5 >= bgaid)
    {
        bga_write_reg(VBE_DISPI_INDEX_ID, bgaid);
        if (bga_read_reg(VBE_DISPI_INDEX_ID) != bgaid)
        {
            return 0;
        }
        return (u32_t)bgaid;
    }
    return 0;
}

u32_t chk_bgamaxver()
{
    bga_write_reg(VBE_DISPI_INDEX_ID, BGA_DEV_ID5);
    if (bga_read_reg(VBE_DISPI_INDEX_ID) == BGA_DEV_ID5)
    {
        return (u32_t)BGA_DEV_ID5;
    }
    bga_write_reg(VBE_DISPI_INDEX_ID, BGA_DEV_ID4);
    if (bga_read_reg(VBE_DISPI_INDEX_ID) == BGA_DEV_ID4)
    {
        return (u32_t)BGA_DEV_ID4;
    }
    bga_write_reg(VBE_DISPI_INDEX_ID, BGA_DEV_ID3);
    if (bga_read_reg(VBE_DISPI_INDEX_ID) == BGA_DEV_ID3)
    {
        return (u32_t)BGA_DEV_ID3;
    }
    bga_write_reg(VBE_DISPI_INDEX_ID, BGA_DEV_ID2);
    if (bga_read_reg(VBE_DISPI_INDEX_ID) == BGA_DEV_ID2)
    {
        return (u32_t)BGA_DEV_ID2;
    }
    bga_write_reg(VBE_DISPI_INDEX_ID, BGA_DEV_ID1);
    if (bga_read_reg(VBE_DISPI_INDEX_ID) == BGA_DEV_ID1)
    {
        return (u32_t)BGA_DEV_ID1;
    }
    bga_write_reg(VBE_DISPI_INDEX_ID, BGA_DEV_ID0);
    if (bga_read_reg(VBE_DISPI_INDEX_ID) == BGA_DEV_ID0)
    {
        return (u32_t)BGA_DEV_ID0;
    }
    return 0;
}

void init_bgadevice(machbstart_t *mbsp)
{
    u32_t retdevid = get_bgadevice();
    if (0 == retdevid)
    {
        return;
    }
    retdevid = chk_bgamaxver();
    if (0 == retdevid)
    {
        return;
    }
    bga_write_reg(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    bga_write_reg(VBE_DISPI_INDEX_XRES, 1024);
    bga_write_reg(VBE_DISPI_INDEX_YRES, 768);
    bga_write_reg(VBE_DISPI_INDEX_BPP, 0x20);
    bga_write_reg(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | (VBE_DISPI_LFB_ENABLED));
    mbsp->mb_ghparm.gh_mode = BGAMODE;
    mbsp->mb_ghparm.gh_vbemodenr = retdevid;
    mbsp->mb_ghparm.gh_x = 1024;
    mbsp->mb_ghparm.gh_y = 768;
    mbsp->mb_ghparm.gh_framphyadr = 0xe0000000;
    mbsp->mb_ghparm.gh_onepixbits = 0x20;
    mbsp->mb_ghparm.gh_bank = 4;
    mbsp->mb_ghparm.gh_curdipbnk = 0;
    mbsp->mb_ghparm.gh_nextbnk = 0;
    mbsp->mb_ghparm.gh_banksz = (mbsp->mb_ghparm.gh_x * mbsp->mb_ghparm.gh_x * 4);
    //test_bga();
    return;
}

void test_bga()
{
    int *p = (int *)(0xe0000000);
    int *p2 = (int *)(0xe0000000 + (1024 * 768 * 4));
    int *p3 = (int *)(0xe0000000 + (1024 * 768 * 4) * 2);

    for (int i = 0; i < (1024 * 768); i++)
    {
        p2[i] = 0x00ff00ff;
    }

    for (int i = 0; i < (1024 * 768); i++)
    {
        p[i] = 0x0000ff00;
    }
    for (int i = 0; i < (1024 * 768); i++)
    {
        p3[i] = 0x00ff0000;
    }
    for (;;)
    {
        bga_write_reg(VBE_DISPI_INDEX_X_OFFSET, 0);
        bga_write_reg(VBE_DISPI_INDEX_Y_OFFSET, 0);
        bga_write_reg(VBE_DISPI_INDEX_VIRT_WIDTH, 1024);
        bga_write_reg(VBE_DISPI_INDEX_VIRT_HEIGHT, 768);
        die(0x400);
        bga_write_reg(VBE_DISPI_INDEX_X_OFFSET, 0);
        bga_write_reg(VBE_DISPI_INDEX_Y_OFFSET, 768);
        bga_write_reg(VBE_DISPI_INDEX_VIRT_WIDTH, 1024);
        bga_write_reg(VBE_DISPI_INDEX_VIRT_HEIGHT, 768 * 2);
        die(0x400);
        bga_write_reg(VBE_DISPI_INDEX_X_OFFSET, 0);
        bga_write_reg(VBE_DISPI_INDEX_Y_OFFSET, 768 * 2);
        bga_write_reg(VBE_DISPI_INDEX_VIRT_WIDTH, 1024);
        bga_write_reg(VBE_DISPI_INDEX_VIRT_HEIGHT, 768 * 3);
        die(0x400);
    }

    for (;;)
        ;
    return;
}

/*功能: 通过陷入realintsve.asm内部汇编 中断 获取 某个具体的VBE模式信息
*/
void get_vbemodeinfo(machbstart_t *mbsp)
{
    realadr_call_entry(RLINTNR(3), 0, 0);
    vbeominfo_t *vomif = (vbeominfo_t *)VBEMINFO_ADR;
    u32_t x = vomif->XResolution, y = vomif->YResolution;
    u32_t *phybass = (u32_t *)(vomif->PhysBasePtr);
    if (vomif->BitsPerPixel < 24)
    {
        kerror("vomif->BitsPerPixel!=32");
    }
    if (x != 1024 || y != 768)
    {
        kerror("xy not");
    }
    if ((u32_t)phybass < 0x100000)
    {
        kerror("phybass not");
    }
    mbsp->mb_ghparm.gh_x = vomif->XResolution;
    mbsp->mb_ghparm.gh_y = vomif->YResolution;
    mbsp->mb_ghparm.gh_framphyadr = vomif->PhysBasePtr;
    mbsp->mb_ghparm.gh_onepixbits = vomif->BitsPerPixel;
    mbsp->mb_ghparm.gh_vmifphyadr = VBEMINFO_ADR;
    m2mcopy(vomif, &mbsp->mb_ghparm.gh_vminfo, sizeof(vbeominfo_t));

    return;
}

/*功能: 通过函数调用 调用realintsve.asm 汇编代码中函数表索引为 4的函数来设置vbe模式
*/
void set_vbemodeinfo()
{
    realadr_call_entry(RLINTNR(4), 0, 0);
    return;
}

u32_t utf8_to_unicode(utf8_t *utfp, int *retuib)
{
    u8_t uhd = utfp->utf_b1, ubyt = 0;
    u32_t ucode = 0, tmpuc = 0;
    if (0x80 > uhd) //0xbf&&uhd<=0xbf
    {
        ucode = utfp->utf_b1 & 0x7f;
        *retuib = 1;
        return ucode;
    }
    if (0xc0 <= uhd && uhd <= 0xdf) //0xdf
    {
        ubyt = utfp->utf_b1 & 0x1f;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b2 & 0x3f;
        ucode = (tmpuc << 6) | ubyt;
        *retuib = 2;
        return ucode;
    }
    if (0xe0 <= uhd && uhd <= 0xef) //0xef
    {
        ubyt = utfp->utf_b1 & 0x0f;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b2 & 0x3f;
        tmpuc <<= 6;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b3 & 0x3f;
        ucode = (tmpuc << 6) | ubyt;
        *retuib = 3;
        return ucode;
    }
    if (0xf0 <= uhd && uhd <= 0xf7) //0xf7
    {
        ubyt = utfp->utf_b1 & 0x7;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b2 & 0x3f;
        tmpuc <<= 6;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b3 & 0x3f;
        tmpuc <<= 6;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b4 & 0x3f;
        ucode = (tmpuc << 6) | ubyt;
        *retuib = 4;
        return ucode;
    }
    if (0xf8 <= uhd && uhd <= 0xfb) //0xfb
    {
        ubyt = utfp->utf_b1 & 0x3;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b2 & 0x3f;
        tmpuc <<= 6;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b3 & 0x3f;
        tmpuc <<= 6;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b4 & 0x3f;
        tmpuc <<= 6;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b5 & 0x3f;
        ucode = (tmpuc << 6) | ubyt;
        *retuib = 5;
        return ucode;
    }
    if (0xfc <= uhd && uhd <= 0xfd) //0xfd
    {
        ubyt = utfp->utf_b1 & 0x1;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b2 & 0x3f;
        tmpuc <<= 6;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b3 & 0x3f;
        tmpuc <<= 6;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b4 & 0x3f;
        tmpuc <<= 6;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b5 & 0x3f;
        tmpuc <<= 6;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b6 & 0x3f;
        ucode = (tmpuc << 6) | ubyt;
        *retuib = 6;
        return ucode;
    }
    *retuib = 0;
    return 0;
}
