#include "rk3568_common.h"
#include "include/mm.h"
#include <arch/arm64/debug_uart.h>
#include <kernel.h>
#include "stdarg.h"
#include <kernel_internal.h>

#define MY_GET_EL(mode)		(((mode) >> 0x02) & 0x03)
#define SYS_GRF					0xFDC60000
#define GRF_IOFUNC_SEL3			0x030C
#define PMU_GRF					0xfdc20000
#define GRF_GPIO0D_IOMUX_L		0x0018
#define CONFIG_DEBUG_UART_BASE 	0xFE660000
#define CONFIG_DEBUG_UART_CLOCK 24000000
#define CONFIG_BAUDRATE 		1500000
#define  MAX_NUMBER_BYTES  64

#define IRAM_START_ADDR					0xfdcc0000
#define BROM_BOOTSOURCE_ID_ADDR 		(IRAM_START_ADDR + 0x10)
#define CONFIG_ROCKCHIP_BOOT_MODE_REG	0xfdc20200 // 0xfdc20000 + 0x200  (PMU_GRF + PMU_GRF_OS_REG0)
#define BROMA_GOTO_NEXT	0
#define BROMA_RELOAD_ME	1
#define reg_per		13

uint64_t debug_print_reg[10000];
int count_offset=2;
K_KERNEL_STACK_DEFINE(earlyprint_stack, PRINTK_STACK_SIZE);
extern u8 BootromContex;
u32 boot_mode, boot_dvid;
u32 bootrom_action;
const unsigned char hex_tab[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

void save_boot_params_ret(void);
int setjmp(u8* ctx);
void longjmp(u8* ctx, int ret);
void printascii(const char *str);
int debug_earlyprint(const char *fmt, ...);


static inline unsigned int get_current_el(void)
{
	return MY_GET_EL(read_currentel());
}

static void ns16650_putc(int ch)
{
	if (ch == '\n') ns16650_putc('\r');

	struct NS16550 *com_port;

	com_port = (struct NS16550 *)CONFIG_DEBUG_UART_BASE;

	while (!(serial_din(&com_port->lsr) & 0x20))
		;
	serial_dout(&com_port->thr, ch);
}


int save_boot_params(void)
{
	int tmp = setjmp(&BootromContex);  // setjmp 首次返回0， 当在别的地方调用 longjmp(brom_ctx, 整数n) 时 setjmp 会再次从这里开始往运行，返回的 tmp 值为 整数n！

	if(0 == tmp)
	{
		// contex saved in setjmp(&BootromContex) & return 0;

		boot_mode = readl(CONFIG_ROCKCHIP_BOOT_MODE_REG);
		boot_dvid = readl(BROM_BOOTSOURCE_ID_ADDR);
		bootrom_action = BROMA_GOTO_NEXT;

		save_boot_params_ret(); // 不让这个函数结束，实际就是从 rk3568tpl_start.S 中的 save_boot_params_ret 处接着往下运行
	}else{
		printascii("longjpm ok.\r\n");

		return bootrom_action;
	}
	// 这里开始的代码，不会被执行！因为根本不会运行到这里！
	return 2;
}

void back_to_bootrom(int exitCode)
{
	bootrom_action = exitCode;

	longjmp(&BootromContex, 888888);
}


void uart_init()
{
	// IOMUX 配置，将UART2对应的GPIO引脚选用为 uart2 mux0 功能
	//*((u32 *)(SYS_GRF + GRF_IOFUNC_SEL3))		= 0x0C000000;
	rk_clrsetreg(SYS_GRF + GRF_IOFUNC_SEL3, 0x0C00, 0x0000);

	// GPIO0_D0 做 uart2 mux0 的 rx 线， GPIO0_D1 做 uart2 mux0 的 tx 线
	//*((u32 *)(PMU_GRF + GRF_GPIO0D_IOMUX_L))	= 0x00770011;
	rk_clrsetreg(PMU_GRF + GRF_GPIO0D_IOMUX_L, 0x0077, 0x0011);

	// 时钟频率与波特率换算？？
	int baud_divisor = DIV_ROUND_CLOSEST(CONFIG_DEBUG_UART_CLOCK, 16 * CONFIG_BAUDRATE);


	struct NS16550 *com_port = (struct NS16550 *)CONFIG_DEBUG_UART_BASE;
	serial_dout(&com_port->ier, (1 << 6));
	serial_dout(&com_port->mcr, 0x03);
	serial_dout(&com_port->fcr, 0x07);

	serial_dout(&com_port->lcr, 0x80 | 0x03);
	serial_dout(&com_port->dll, baud_divisor & 0xff);
	serial_dout(&com_port->dlm, (baud_divisor >> 8) & 0xff);
	serial_dout(&com_port->lcr, 0x03);

}

static int outc(int c)
{
    ns16650_putc(c);
    return 0;
}

static int outs (const char *s)
{
    while (*s != '\0')
        ns16650_putc(*s++);
    return 0;
}

static int out_num(long n, int base, char lead, int maxwidth)
{
    unsigned long m = 0;
    char buf[MAX_NUMBER_BYTES], *s = buf + sizeof(buf);
    int count = 0, i = 0;

    *--s = '\0';

    if (n < 0)
        m = -n;
    else
        m = n;

    do
    {
        *--s = hex_tab[m % base];
        count++;
    }
    while ((m /= base) != 0);

    if( maxwidth && count < maxwidth)
    {
        for (i = maxwidth - count; i; i--)
            *--s = lead;
    }

    if (n < 0)
        *--s = '-';

    return outs(s);
}

/*ref: int vdebug_earlyprint(const char *format, va_list ap); */
static int my_vdebug_earlyprint(const char *fmt, va_list ap)
{
    char lead = ' ';
    int  maxwidth = 0;

    for(; *fmt != '\0'; fmt++)
    {
        if (*fmt != '%')
        {
            outc(*fmt);
            continue;
        }
        lead = ' ';
        maxwidth = 0;

        //format : %08d, %8d,%d,%u,%x,%f,%c,%s
        fmt++;
        if(*fmt == '0')
        {
            lead = '0';
            fmt++;
        }

        while(*fmt >= '0' && *fmt <= '9')
        {
            maxwidth *= 10;
            maxwidth += (*fmt - '0');
            fmt++;
        }

        switch (*fmt)
        {
        case 'd':
            out_num(va_arg(ap, int),          10, lead, maxwidth);
            break;
        case 'o':
            out_num(va_arg(ap, unsigned int),  8, lead, maxwidth);
            break;
        case 'u':
            out_num(va_arg(ap, unsigned int), 10, lead, maxwidth);
            break;
        case 'x':
            out_num(va_arg(ap, unsigned int), 16, lead, maxwidth);
            break;
        case 'c':
            outc(va_arg(ap, int   ));
            break;
        case 's':
            outs(va_arg(ap, char *));
            break;

        default:
            outc(*fmt);
            break;
        }
    }
    return 0;
}

int debug_earlyprint(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    my_vdebug_earlyprint(fmt, ap);
    va_end(ap);
    return 0;
}


void printascii(const char *str)
{
	while (*str) ns16650_putc(*str++);
}


static void asm_register_print(void)
{

	switch(get_current_el()){
	case 2: {
		debug_earlyprint("sctlr_el2:\t 0x%08x-%08x,\t spsr_el2:\t 0x%08x-%08x  \r\n", (unsigned long)(read_sctlr_el2()>>32),read_sctlr_el2(), (unsigned long)(read_spsr_el2()>>32), read_spsr_el2());
		debug_earlyprint("mair_el2:\t 0x%08x-%08x,\t tcr_el2:\t 0x%08x-%08x  \r\n", (unsigned long)(read_mair_el2()>>32),read_mair_el2(), (unsigned long)(read_tcr_el2()>>32),read_tcr_el2());
		debug_earlyprint("vtcr_el2:\t 0x%08x-%08x,\t vttbr_el2:\t 0x%08x-%08x  \r\n", (unsigned long)(read_vtcr_el2()>>32), read_vtcr_el2(), (unsigned long)(read_vttbr_el2()>>32), read_vttbr_el2());
		debug_earlyprint("tpidr_el2:\t 0x%08x-%08x,\t ttbr0_el2:\t 0x%08x-%08x  \r\n", (unsigned long)(read_tpidr_el2()>>32), read_tpidr_el2(), (unsigned long)(read_ttbr0_el2()>>32), read_ttbr0_el2());
		debug_earlyprint("vbar_el2:\t 0x%08x-%08x,\t mpidr_el1:\t 0x%08x-%08x  \r\n", (unsigned long)(read_vbar_el2()>>32), read_vbar_el2(), (unsigned long)(read_mpidr_el1()>>32), read_mpidr_el1());
		debug_earlyprint("cptr_el2:\t 0x%08x-%08x,\t hcr_el2:\t 0x%08x-%08x  \r\n", (unsigned long)(read_cptr_el2()>>32), read_cptr_el2(), (unsigned long)(read_hcr_el2()>>32), read_hcr_el2());
		debug_earlyprint("esr_el2:\t 0x%08x-%08x,\t elr_el2:\t 0x%08x-%08x \r\n", (unsigned long)(read_esr_el2()>>32), read_esr_el2(), (unsigned long)(read_elr_el2()>>32),read_elr_el2());
	}
	case 1: {
		debug_earlyprint("sctlr_el1:\t 0x%08x-%08x,\t spsr_el1:\t 0x%08x-%08x  \r\n", (unsigned long)(read_sctlr_el1()>>32),read_sctlr_el1(), (unsigned long)(read_spsr_el1()>>32),read_spsr_el1());
		debug_earlyprint("mair_el1:\t 0x%08x-%08x,\t tcr_el1:\t 0x%08x-%08x  \r\n", (unsigned long)(read_mair_el1()>>32),read_mair_el1(), (unsigned long)(read_tcr_el1()>>32),read_tcr_el1());
		debug_earlyprint("ttbr0_el1:\t 0x%08x-%08x,\t ttbr1_el1:\t 0x%08x-%08x  \r\n", (unsigned long)(read_ttbr0_el1()>>32),read_ttbr0_el1(), (unsigned long)(read_ttbr1_el1()>>32),read_ttbr1_el1());
		debug_earlyprint("mpidr_el1:\t 0x%08x-%08x,\t cpacr_el1:\t 0x%08x-%08x \r\n",  (unsigned long)(read_mpidr_el1()>>32),read_mpidr_el1(), (unsigned long)(read_cpacr_el1()>>32),read_cpacr_el1());
		debug_earlyprint("esr_el1:\t 0x%08x-%08x,\t elr_el1:\t 0x%08x-%08x \r\n",  (unsigned long)(read_esr_el1()>>32),read_esr_el1(), (unsigned long)(read_elr_el1()>>32),read_elr_el1());
		debug_earlyprint("isr_el1:\t 0x%08x-%08x,\t \r\n",  (unsigned long)(read_isr_el1()>>32),read_isr_el1());
		break;
	}
	default:
		printascii("Wrong exception mode! \n");
		break;
	}
	return;
}

void asm_register_print_smp(unsigned long long reg0, unsigned long long reg1)
{
	unsigned long reg0_1, reg1_1;

	reg0_1 = reg0 >> 32;
	reg1_1 = reg1 >> 32;
	printk("x0: 0x%08x-%08x , x1: 0x%08x-%08x  \r\n", reg0_1, reg0, reg1_1, reg1);

	switch(get_current_el()){
	case 2: {
		printk("sctlr_el2:\t 0x%08x-%08x,\t spsr_el2:\t 0x%08x-%08x  \r\n", (unsigned long)(read_sctlr_el2()>>32),read_sctlr_el2(), (unsigned long)(read_spsr_el2()>>32), read_spsr_el2());
		printk("mair_el2:\t 0x%08x-%08x,\t tcr_el2:\t 0x%08x-%08x  \r\n", (unsigned long)(read_mair_el2()>>32),read_mair_el2(), (unsigned long)(read_tcr_el2()>>32),read_tcr_el2());
		printk("vtcr_el2:\t 0x%08x-%08x,\t vttbr_el2:\t 0x%08x-%08x  \r\n", (unsigned long)(read_vtcr_el2()>>32), read_vtcr_el2(), (unsigned long)(read_vttbr_el2()>>32), read_vttbr_el2());
		printk("tpidr_el2:\t 0x%08x-%08x,\t ttbr0_el2:\t 0x%08x-%08x  \r\n", (unsigned long)(read_tpidr_el2()>>32), read_tpidr_el2(), (unsigned long)(read_ttbr0_el2()>>32), read_ttbr0_el2());
		printk("vbar_el2:\t 0x%08x-%08x,\t mpidr_el1:\t 0x%08x-%08x  \r\n", (unsigned long)(read_vbar_el2()>>32), read_vbar_el2(), (unsigned long)(read_mpidr_el1()>>32), read_mpidr_el1());
		printk("cptr_el2:\t 0x%08x-%08x,\t hcr_el2:\t 0x%08x-%08x  \r\n", (unsigned long)(read_cptr_el2()>>32), read_cptr_el2(), (unsigned long)(read_hcr_el2()>>32), read_hcr_el2());
		printk("esr_el2:\t 0x%08x-%08x\r\n", (unsigned long)(read_esr_el2()>>32), read_esr_el2());
	}
	case 1: {
		printk("sctlr_el1:\t 0x%08x-%08x,\t spsr_el1:\t 0x%08x-%08x  \r\n", (unsigned long)(read_sctlr_el1()>>32),read_sctlr_el1(), (unsigned long)(read_spsr_el1()>>32),read_spsr_el1());
		printk("mair_el1:\t 0x%08x-%08x,\t tcr_el1:\t 0x%08x-%08x  \r\n", (unsigned long)(read_mair_el1()>>32),read_mair_el1(), (unsigned long)(read_tcr_el1()>>32),read_tcr_el1());
		printk("ttbr0_el1:\t 0x%08x-%08x,\t ttbr1_el1:\t 0x%08x-%08x  \r\n", (unsigned long)(read_ttbr0_el1()>>32),read_ttbr0_el1(), (unsigned long)(read_ttbr1_el1()>>32),read_ttbr1_el1());
		printk("mpidr_el1:\t 0x%08x-%08x,\t cpacr_el1:\t 0x%08x-%08x \r\n",  (unsigned long)(read_mpidr_el1()>>32),read_mpidr_el1(), (unsigned long)(read_cpacr_el1()>>32),read_cpacr_el1());
		printk("esr_el1:\t 0x%08x-%08x,\t elr_el1:\t 0x%08x-%08x \r\n",  (unsigned long)(read_esr_el1()>>32),read_esr_el1(), (unsigned long)(read_elr_el1()>>32),read_elr_el1());
		printk("isr_el1:\t 0x%08lx-%08llx,\t tpidrro_el0:\t 0x%08lx-%08llx\r\n",  (unsigned long)(read_isr_el1()>>32),read_isr_el1(), 
		(unsigned long)(read_tpidrro_el0()>>32),read_tpidrro_el0());
		break;
	}
	default:
		printascii("Wrong exception mode! \n");
		break;
	}
	return;
}

static void show_boot_mode(void)
{
	printascii("**********************************");
	printascii("\n## Booting on uboot......");
	printascii("\n## Earlyprintk init successful!");
	switch(get_current_el()){
	case 2: {
		printascii("\n## Boot system on EL2 mode!\n");
		break;
	}
	case 1: {
		printascii("\n## Boot system on EL1 mode!\n");
		break;
	}
	default:
		printascii("Boot Wrong mode! \n");
		break;
	}
	return;
}

void early_print_debug(unsigned long long reg1, unsigned long long reg2)
{
	unsigned long reg1_1, reg2_1;

	reg1_1 = reg1 >> 32;
	reg2_1 = reg2 >> 32;
	debug_earlyprint("x0: 0x%08x-%08x , x1: 0x%08x-%08x  \r\n", reg1_1, reg1, reg2_1, reg2);
//	asm_register_print();
	return;
}

void show_print_debug_delay(void)
{
	int j;
	while(count_offset>0){
		for(j=count_offset-reg_per; j<count_offset; j+=2){
			debug_earlyprint("Reg-%d: \t 0x%08x-%08x,\t Reg-%d: \t 0x%08x-%08x  \r\n", j+reg_per-count_offset, 
				(unsigned long)(debug_print_reg[j]>>32),debug_print_reg[j], j+1+reg_per-count_offset,(unsigned long)(debug_print_reg[j+1]>>32), debug_print_reg[j+1]);
		}
		debug_earlyprint("---------------------------------------\n");
	count_offset -= reg_per;
	}
}

int tpl_main(void)
{
	uart_init();
	show_boot_mode();

	return 0;
}
