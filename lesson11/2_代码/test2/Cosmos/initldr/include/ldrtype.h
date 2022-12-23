#ifndef _LDRTYPE_H
#define _LDRTYPE_H


#define BFH_RW_R 1
#define BFH_RW_W 2

#define BFH_BUF_SZ 0x1000
#define BFH_ONERW_SZ 0x1000
#define BFH_RWONE_OK 1
#define BFH_RWONE_ER 2
#define BFH_RWALL_OK 3

#define FHDSC_NMAX 192 //文件名 长度
#define FHDSC_SZMAX 256
#define MDC_ENDGIC 0xaaffaaffaaffaaff
#define MDC_RVGIC 0xffaaffaaffaaffaa
#define REALDRV_PHYADR 0x1000
#define ILDRKRL_PHYADR 0x200000
#define IMGSHEL_PHYADR 0x30000
#define IKSTACK_PHYADR (0x90000-0x10)
#define IKSTACK_SIZE 0x1000
#define IMGFILE_PHYADR 0x4000000
#define IMGKRNL_PHYADR 0x2000000
#define KINITPAGE_PHYADR 0x1000000
#define KINITFRVM_PHYADR 0x800000
#define KINITFRVM_SZ 0x400000
#define LDRFILEADR IMGFILE_PHYADR
#define MLOSDSC_OFF (0x1000)
#define MRDDSC_ADR (mlosrddsc_t*)(LDRFILEADR+0x1000)

#define KRNL_VIRTUAL_ADDRESS_START 0xffff800000000000
#define KPML4_P (1<<0)
#define KPML4_RW (1<<1)
#define KPML4_US (1<<2)
#define KPML4_PWT (1<<3)
#define KPML4_PCD (1<<4)
#define KPML4_A (1<<5)

#define KPDPTE_P (1<<0)
#define KPDPTE_RW (1<<1)
#define KPDPTE_US (1<<2)
#define KPDPTE_PWT (1<<3)
#define KPDPTE_PCD (1<<4)
#define KPDPTE_A (1<<5)

#define KPDE_P (1<<0)
#define KPDE_RW (1<<1)
#define KPDE_US (1<<2)
#define KPDE_PWT (1<<3)
#define KPDE_PCD (1<<4)
#define KPDE_A (1<<5)
#define KPDE_D (1<<6)
#define KPDE_PS (1<<7)
#define KPDE_G (1<<8)
#define KPDE_PAT (1<<12)

#define KPML4_SHIFT 39
#define KPDPTTE_SHIFT 30
#define KPDP_SHIFT 21
#define PGENTY_SIZE 512


// 文件头描述符
typedef struct s_fhdsc
{
    u64_t fhd_type;//文件类型
    u64_t fhd_subtype; //文件子类型
    u64_t fhd_stuts; // 文件状态
    u64_t fhd_id; //文件id
    u64_t fhd_intsfsoff; //文件在映像文件位置开始偏移
    u64_t fhd_intsfend; //文件在映像文件的结束偏移
    u64_t fhd_frealsz;// 文件是实际大小
    u64_t fhd_fsum; // 文件校验和
    char   fhd_name[FHDSC_NMAX]; // 文件名字
}fhdsc_t;

//映像文件 文件头描述符
typedef struct s_mlosrddsc
{
    u64_t mdc_mgic; //映射文件标识 , 魔数
    u64_t mdc_sfsum; //未使用
    u64_t mdc_sfsoff; //未使用
    u64_t mdc_sfeoff; // 未使用
    u64_t mdc_sfrlsz; //未使用 

    u64_t mdc_ldrbk_s; //映像文件中二级引导器的开始偏移
    u64_t mdc_ldrbk_e; //映像文件中二级引导器的结束偏移
    u64_t mdc_ldrbk_rsz; //映像文件中二级引导器的实际大小
    u64_t mdc_ldrbk_sum; //映像文件中二级引导器的校验和

    u64_t mdc_fhdbk_s; //映像文件中 文件头描述的 开始偏移
    u64_t mdc_fhdbk_e; //映像文件中 文件头描述的 结束偏移
    u64_t mdc_fhdbk_rsz; // 映像文件中 文件头描述的 实际大小
    u64_t mdc_fhdbk_sum; // 映像文件中 文件头描述的 校验和 

    u64_t mdc_filbk_s; // 映像文件中 文件数据的开始偏移
    u64_t mdc_filbk_e; // 映像文件中 文件数据的 结束偏移
    u64_t mdc_filbk_rsz; // 映像文件中 文件数据的 实际大小
    u64_t mdc_filbk_sum; // 映像文件中 文件数据的 校验和
    u64_t mdc_ldrcodenr; // 映像文件中 二级引导器的文件头描述符 的 索引号
    u64_t mdc_fhdnr; //映像文件中 文件头描述符 有多少个
    u64_t mdc_filnr; // 映像文件中 文件头有多少个
    u64_t mdc_endgic; // 影响文件结束标识
    u64_t mdc_rv; //映像文件版本
}mlosrddsc_t;

#define RLINTNR(x) (x*2)

typedef struct s_DPT
{
    u8_t dp_bmgic;
    u8_t dp_pshead;
    u8_t dp_pssector;
    u8_t dp_pscyder;
    u8_t dp_ptype;
    u8_t dp_pehead;
    u8_t dp_pesector;
    u8_t dp_pecyder;
    u32_t dp_pslba;
    u32_t dp_plbasz;

}__attribute__((packed)) dpt_t;


typedef struct s_MBR
{
    char_t mb_byte[446];
    dpt_t mb_part[4];
    u16_t mb_endmgic;
}__attribute__((packed)) mbr_t;
typedef struct s_EBR
{
    char_t eb_byte[446];
    dpt_t eb_part[4];
    u16_t eb_endmgic;
}__attribute__((packed)) ebr_t;

typedef struct s_IDTR
{
        u16_t idtlen;
        u32_t idtbas;
}idtr_t;



typedef struct s_RWHDPACK
{

    u8_t rwhpk_sz;
    u8_t rwhpk_rv;
    u8_t rwhpk_sn;
    u8_t rwhpk_rv1;
    u16_t rwhpk_of;
    u16_t rwhpk_se;
    u32_t rwhpk_ll;
    u32_t rwhpk_lh;

}__attribute__((packed)) rwhdpach_t;
#define RAM_USABLE 1 //可用内存
#define RAM_RESERV 2 //保留内存 禁止使用
#define RAM_ACPIREC 3 //ACPI 表相关的
#define RAM_ACPINVS 4 //ACPI NVS 空间
#define RAM_AREACON 5 //包含坏内存

/*
   GNU C 的一大特色就是__attribute__ 机制。
   __attribute__ 可以设置函数属性（Function Attribute ）、变量属性（Variable Attribute ）和类型属性（Type Attribute ）。 
   属性:   
    1. aligned  : 该属性设定一个指定大小的对齐格式（以字节 为单位），例如：typedef int int32_t __attribute__ ((aligned (8)));
    2. packed  :使用该属性对struct 或者union 类型进行定义，设定其类型的每一个变量的内存约束。
            --> 紧凑的内存布局
    csdn:https://tianyalu.blog.csdn.net/article/details/108748430?spm=1001.2101.3001.6650.13&utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogCommendFromBaidu%7ERate-13-108748430-blog-96713277.pc_relevant_3mothn_strategy_and_data_recovery&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogCommendFromBaidu%7ERate-13-108748430-blog-96713277.pc_relevant_3mothn_strategy_and_data_recovery&utm_relevant_index=18
*/
typedef struct s_e820{
    u64_t saddr;    /* 内存开始地址 */
    u64_t lsize;    /* 内存大小 */
    u32_t type;    /* 内存类型 */
}__attribute__((packed)) e820map_t;

/*功能:VBE模式信息存放结构体
http://www.javashuo.com/article/p-ovzlljfs-cs.html
*/
typedef struct s_VBEINFO
{
        char vbesignature[4];// VBE标识符 应该是 "VESA"
        u16_t vbeversion; //VBE 版本
        u32_t oemstringptr;//指向 OEM 信息的远指针，selector : offset 形式，用于识别图形控制器芯片、驱动等
        u32_t capabilities;//图形控制器的功能
        u32_t videomodeptr; //是指向 "图形模式" 列表的指针，与oem_string_ptr类型一样 
                    // ===> 暨 一大堆不同分辨率 的模式 
        u16_t totalmemory;//是64kb内存块的个数
        u16_t oemsoftwarerev; //VBE实现软件修订
        u32_t oemvendornameptr; //VbeFarPtr的供应商名称字符串
        u32_t oemproductnameptr; //VbeFarPtr的产品修订字符串 
        u32_t oemproductrevptr;// VbeFarPtr的产品名称字符串
        u8_t reserved[222]; //为VBE实现暂存区域保留
        u8_t oemdata[256]; //OEM字符串数据区（V2.0+支持）
}__attribute__((packed)) vbeinfo_t;

/*功能: 具体的某一个编号下的VBE模式信息 ,存储的结构体
    eg: 0x118 编号vbe模式信息
    参考链接:http://t.zoukankan.com/zyl910-p-2186633.html
*/
typedef struct s_VBEOMINFO
{
    u16_t ModeAttributes; //模式属性 
    /*第 4 位为模式位：1 = Graphics mode 图形，0 = Text mode 文本。
    第 7 位为缓冲模式位：1 = Linear frame buffer mode 线性帧，0 = Nonlinear frame buffer mode 非线性帧。
    */
    u8_t  WinAAttributes; /*窗口 A 的属性*/
    u8_t  WinBAttributes; /*窗口 B 的属性*/
    u16_t WinGranularity; /*窗口粒度*/
    u16_t WinSize; /*窗口大小*/
    u16_t WinASegment; /*窗口 A 起始段地址*/
    u16_t WinBSegment; /*窗口 B 起始段地址*/
    u32_t WinFuncPtr; /*窗口函数的实模式指针*/
    u16_t BytesPerScanLine;/*每扫描行字节数*/


    /*以下是 : VBE 1.2及以上版本的必填信息 */
    u16_t XResolution; /*x分辨率*/
    u16_t YResolution; /*y分辨率*/
    u8_t  XCharSize; /*字符单元宽度（像素）*/
    u8_t  YCharSize; /*字符单元高度（像素）*/
    u8_t  NumberOfPlanes; /*内存平面数*/
    u8_t  BitsPerPixel; /*每一个像素所占的为数*/
    u8_t  NumberOfBanks; /*显示段个数*/
    u8_t  MemoryModel; /*存储器模式类型*/
    u8_t  BankSize; /*显示段的大小,以KB为单位*/
    u8_t  NumberOfImagePages; /*可以同时载入的最大满屏图像数*/
    u8_t  Reserved; /*为页面功能保留*/
    u8_t  RedMaskSize; /*红色所占为数*/
    u8_t  RedFieldPosition; /*红色最低有效位位置*/
    u8_t  GreenMaskSize; /*绿色所占的为数*/
    u8_t  GreenFieldPosition; /*绿色最低的有效位位置*/
    u8_t  BlueMaskSize; /*蓝色所占的位数*/
    u8_t  BlueFieldPosition; /*蓝色最低的有效位位置*/
    u8_t  RsvdMaskSize; /*保留色所占的位置*/
    u8_t  RsvdFieldPosition; /*保留色最低的有效位位置*/
    u8_t  DirectColorModeInfo; /*直接颜色模式的属性*/
    /*以下是: VBE 2.0 及以上的必填信息*/
    u32_t PhysBasePtr; /*平坦内存帧缓冲区的物理地址，即显存起始地址*/
    u32_t Reserved1; /*保留-始终设置为0 */
    u16_t Reserved2; /*保留 始终设置为0*/
    u16_t LinBytesPerScanLine; /*线性模式的每条扫描线字节数 */
    u8_t  BnkNumberOfImagePages; /*使用窗口功能时,使用的页面数*/
    u8_t  LinNumberOfImagePages; /*使用大的线性缓冲区时显示的页面数*/
    u8_t  LinRedMaskSize; /*使用大的线性缓冲区时,红色所占的位数*/
    u8_t  LinRedFieldPosition; /*使用大的线性缓冲区时,红色的最低有效位*/
    u8_t  LinGreenMaskSize; /*同上 绿色*/
    u8_t  LinGreenFieldPosition;
    u8_t  LinBlueMaskSize;
    u8_t  LinBlueFieldPosition;
    u8_t  LinRsvdMaskSize; /*保留色*/
    u8_t  LinRsvdFieldPosition;
    u32_t MaxPixelClock; /*图形模式的最大像素时钟（Hz)*/
    u8_t  Reserved3[189]; /*保留部分*/
}__attribute__((packed)) vbeominfo_t;

/*每一个像素点的数据结构信息*/
typedef struct s_PIXCL
{
    u8_t cl_b;//蓝色
    u8_t cl_g;//绿色
    u8_t cl_r;//红色
    u8_t cl_a;//透明
}__attribute__((packed)) pixcl_t;
#define BGRA(r,g,b) ((0|(r<<16)|(g<<8)|b))
typedef u32_t pixl_t;
#define VBEMODE 1
#define GPUMODE 2
#define BGAMODE 3
/*功能: 二级引导器中搜寻的图像信息 的存储结构*/
typedef struct s_GRAPH
{
    u32_t gh_mode; /*图形模式*/
    u32_t gh_x; /*图像模式 x轴分辨率*/
    u32_t gh_y; /*图像模式 Y轴 分辨率*/
    u32_t gh_framphyadr; /*显存起始位置 真实物理地址*/
    u32_t gh_onepixbits; /*一个像素点 所占的位数*/
    u32_t gh_vbemodenr; /*vbe模式下,具体的摸一个图像模式的编号*/
    u32_t gh_vifphyadr; /*vbe 控制器 信息 的结构体存放的位置*/
    u32_t gh_vmifphyadr; /*vbe 某个模式的信息结构体存放指针*/
    u32_t gh_bank; /*2022年11月8日18:34:27: 暂时理解 图像模式显示段数*/
    u32_t gh_curdipbnk; 
    u32_t gh_nextbnk;
    u32_t gh_banksz;
    u32_t gh_logophyadrs;
    u32_t gh_logophyadre;
    u32_t gh_fontadr;
    u32_t gh_ftsectadr;
    u32_t gh_ftsectnr;
    u32_t gh_fnthight;
    u32_t gh_nxtcharsx;
    u32_t gh_nxtcharsy;
    u32_t gh_linesz;
    vbeinfo_t gh_vbeinfo; /*vbe 控制器信息结构体地址*/
    vbeominfo_t gh_vminfo; /*vbe 模式信息结构体*/
}__attribute__((packed)) graph_t;

typedef struct s_BMFHEAD
{
    u16_t bf_tag;   //0x4d42
    u32_t bf_size;
    u16_t bf_resd1;
    u16_t bf_resd2;
    u32_t bf_off;
}__attribute__((packed)) bmfhead_t;//14

typedef struct s_BITMINFO
{
    u32_t bi_size;
    s32_t bi_w;
    s32_t bi_h;
    u16_t bi_planes;
    u16_t bi_bcount;
    u32_t bi_comp;
    u32_t bi_szimg;
    s32_t bi_xpelsper;
    s32_t bi_ypelsper;
    u32_t bi_clruserd;
    u32_t bi_clrimport;
}__attribute__((packed)) bitminfo_t;//40

typedef struct s_BMDBGR
{
    u8_t bmd_b;
    u8_t bmd_g;
    u8_t bmd_r;
}__attribute__((packed)) bmdbgr_t;

typedef struct s_UTF8
{
    u8_t utf_b1;
    u8_t utf_b2;
    u8_t utf_b3;
    u8_t utf_b4;
    u8_t utf_b5;
    u8_t utf_b6;
}__attribute__((packed)) utf8_t;

typedef struct s_FONTFHDER
{
    u8_t  fh_mgic[4];
    u32_t fh_fsize;
    u8_t  fh_sectnr;
    u8_t  fh_fhght;
    u16_t fh_wcpflg;
    u16_t fh_nchars;
    u8_t  fh_rev[2];
}__attribute__((packed)) fontfhder_t;

typedef struct s_FTSECTIF
{
    u16_t fsf_fistuc;
    u16_t fsf_lastuc;
    u32_t fsf_off;
}__attribute__((packed)) ftsectif_t;

typedef struct s_UFTINDX
{
    u32_t ui_choff:26;
    u32_t ui_chwith:6;
}__attribute__((packed)) uftindx_t;
#define MAX_CHARBITBY 32
typedef struct s_FNTDATA
{
    u8_t fntwxbyte;
    u8_t fnthx;
    u8_t fntwx;
    u8_t fntchmap[MAX_CHARBITBY];
}__attribute__((packed)) fntdata_t;


typedef struct s_KLFOCPYMBLK
{
    u64_t sphyadr;
    u64_t ocymsz;
}__attribute__((packed)) klfocpymblk_t;
#define MBS_MIGC (u64_t)((((u64_t)'L')<<56)|(((u64_t)'M')<<48)|(((u64_t)'O')<<40)|(((u64_t)'S')<<32)|(((u64_t)'M')<<24)|(((u64_t)'B')<<16)|(((u64_t)'S')<<8)|((u64_t)'P'))

typedef struct s_MRSDP
{
    u64_t rp_sign;
    u8_t rp_chksum;
    u8_t rp_oemid[6];
    u8_t rp_revn;
    u32_t rp_rsdtphyadr;
    u32_t rp_len;
    u64_t rp_xsdtphyadr;
    u8_t rp_echksum;
    u8_t rp_resv[3];
}__attribute__((packed)) mrsdp_t;
/* 
    机器信息结构
*/
typedef struct s_MACHBSTART
{
    u64_t   mb_migc;          //LMOSMBSP//0
    u64_t   mb_chksum;//8
    u64_t   mb_krlinitstack;//16 内核栈地址
    u64_t   mb_krlitstacksz;//24  内核栈大小
    u64_t   mb_imgpadr; // 操作系统映像地址
    u64_t   mb_imgsz; // 操作系统 映像大小
    u64_t   mb_krlimgpadr; // 内核映像文件地址
    u64_t   mb_krlsz; // 内核大小
    u64_t   mb_krlvec;
    u64_t   mb_krlrunmode;
    u64_t   mb_kalldendpadr;
    u64_t   mb_ksepadrs;
    u64_t   mb_ksepadre;
    u64_t   mb_kservadrs;
    u64_t   mb_kservadre;
    u64_t   mb_nextwtpadr; // 下一段空闲内存的地址 
    u64_t   mb_bfontpadr; // 操作系统字体地址
    u64_t   mb_bfontsz; // 操作系统字体大小
    u64_t   mb_fvrmphyadr; //机器显存地址
    u64_t   mb_fvrmsz; // 机器显存大小
    u64_t   mb_cpumode; // 机器cpu工作模式
    u64_t   mb_memsz; // 机器内存大小
    u64_t   mb_e820padr;//机器e820 数组地址
    u64_t   mb_e820nr;//机器e820数组元素个数
    u64_t   mb_e820sz;//机器e820数组大小
    u64_t   mb_e820expadr; // ex 开头应该表示长模式
    u64_t   mb_e820exnr; 
    u64_t   mb_e820exsz; 
    u64_t   mb_memznpadr;
    u64_t   mb_memznnr;
    u64_t   mb_memznsz;
    u64_t   mb_memznchksum;
    u64_t   mb_memmappadr;
    u64_t   mb_memmapnr;
    u64_t   mb_memmapsz;
    u64_t   mb_memmapchksum;
    u64_t   mb_pml4padr; // 机器页表数据地址
    u64_t   mb_subpageslen; // 机器页表个数
    u64_t   mb_kpmapphymemsz;// 操作系统映射空间大侠
    u64_t   mb_ebdaphyadr;
    mrsdp_t mb_mrsdp; 
    graph_t mb_ghparm; //图形信息
}__attribute__((packed)) machbstart_t;

#define MBSPADR ((machbstart_t*)(0x100000))
#define VBE_DISPI_IOPORT_INDEX (0x01CE)
#define VBE_DISPI_IOPORT_DATA (0x01CF)
#define VBE_DISPI_INDEX_ID (0)
#define VBE_DISPI_INDEX_XRES (1)
#define VBE_DISPI_INDEX_YRES (2)
#define VBE_DISPI_INDEX_BPP (3)
#define VBE_DISPI_INDEX_ENABLE (4)
#define VBE_DISPI_INDEX_BANK (5)
#define VBE_DISPI_INDEX_VIRT_WIDTH (6)
#define VBE_DISPI_INDEX_VIRT_HEIGHT (7)
#define VBE_DISPI_INDEX_X_OFFSET (8)
#define VBE_DISPI_INDEX_Y_OFFSET (9)
#define BGA_DEV_ID0 (0xB0C0) //- setting X and Y resolution and bit depth (8 BPP only), banked mode
#define BGA_DEV_ID1 (0xB0C1) //- virtual width and height, X and Y offset0
#define BGA_DEV_ID2 (0xB0C2) //- 15, 16, 24 and 32 BPP modes, support for linear frame buffer, support for retaining memory contents on mode switching
#define BGA_DEV_ID3 (0xB0C3) //- support for getting capabilities, support for using 8 bit DAC
#define BGA_DEV_ID4 (0xB0C4) //- VRAM increased to 8 MB
#define BGA_DEV_ID5 (0xB0C5) //- VRAM increased to 16 MB? [TODO: verify and check for other changes]
#define VBE_DISPI_BPP_4 (0x04)
#define VBE_DISPI_BPP_8 (0x08)
#define VBE_DISPI_BPP_15 (0x0F)
#define VBE_DISPI_BPP_16 (0x10)
#define VBE_DISPI_BPP_24 (0x18)
#define VBE_DISPI_BPP_32 (0x20)
#define VBE_DISPI_DISABLED (0x00)
#define VBE_DISPI_ENABLED (0x01)
#define VBE_DISPI_LFB_ENABLED (0x40)


void REGCALL realadr_call_entry(u16_t callint,u16_t val1,u16_t val2);
#endif // LDRTYPE_H
