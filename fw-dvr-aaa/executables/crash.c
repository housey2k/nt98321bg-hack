// crash.c
int main(void)
{
    //volatile int *p = (int *)0x0;
    //*p = 0xDEAD;

    while (1) {
        asm volatile ("nop");
    }
    return 0;
}
 
