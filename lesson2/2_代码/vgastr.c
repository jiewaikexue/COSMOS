
// jie @ 2022-10-23 15:37:15
void _strwrite(char * string) {
    char * p_strdst = (char *) (0xb8000); //指向显存开始的地址
    while (*string) {
        *p_strdst = *string ++;
	*(p_strdst + 1) = 0x4; 
        p_strdst += 2;//一个字节控制字符,一个字节控制颜色
    }
} 

void printf(char * fmt,...) {
    _strwrite(fmt);
    return;
}
