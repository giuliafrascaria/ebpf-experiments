#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <uapi/linux/bpf.h>
#include <linux/version.h>
#include <linux/ptrace.h>
#include <linux/kernel.h>
#include "bpf_helpers.h"


#define _(P) ({typeof(P) val = 0; bpf_probe_read(&val, sizeof(val), &P); val;})
#define PROG(F) SEC("kprobe/"__stringify(F)) int bpf_func_##F
#define	UBUFFSIZE	9

struct bpf_map_def SEC("maps") my_read_map =
{
	.type = BPF_MAP_TYPE_ARRAY,
	.key_size = sizeof(u32),
	.value_size = sizeof(u64),
	.max_entries = 1,	//used to pass the buffer address from userland
};

struct bpf_map_def SEC("maps") result_map =
{
	.type = BPF_MAP_TYPE_ARRAY,
	.key_size = sizeof(u32),
	.value_size = sizeof(u64),
	.max_entries = 1,	//used to pass the average back to the user
};

struct bpf_map_def SEC("maps") jmp_table = {
	.type = BPF_MAP_TYPE_PROG_ARRAY,
	.key_size = sizeof(u32),
	.value_size = sizeof(u32),
	.max_entries = 8,
};

#define AVG_FUNC 1
#define MAX_FUNC 2
#define MIN_FUNC 3


SEC("kprobe/copyout_bpf")
int bpf_prog7(struct pt_regs *ctx)
{
    char s1[] = "entering modified copyout\n";
	bpf_trace_printk(s1, sizeof(s1));

    bpf_tail_call(ctx, &jmp_table, AVG_FUNC);

	return 0;
}

PROG(AVG_FUNC)(struct pt_regs *ctx)
{
    bpf_my_printk();

	char s1[] = "entering tail call\n";
	bpf_trace_printk(s1, sizeof(s1));


	// instantiate parameters
	void __user *to;
	const void *from;
	int blen;

	//parse parameters from ctx
	to = (void __user *) PT_REGS_PARM1(ctx);
	from = (const void *) PT_REGS_PARM2(ctx);
	blen = PT_REGS_PARM3(ctx);

	//check buffer address
	__u32 key = 0;
	__u64 ** val;
	val = bpf_map_lookup_elem(&my_read_map, &key);

	if(!val)
	{
		char s[] = "error reading buffer value from map, read entry\n";
		bpf_trace_printk(s, sizeof(s)); 
		return 0;
	}

	char str2[] = "buffer on params %lu, buffer on map %lu\n";
	bpf_trace_printk(str2, sizeof(str2), (unsigned long) to, (unsigned long) *val);


	if (to == *val)
	{
		char s2[] = "copyout to 0x%p, ul %lu len %d\n";
		bpf_trace_printk(s2, sizeof(s2), to, (unsigned long) to, blen);

	
		char userbuff[UBUFFSIZE];

		//char singlechar;
		//singlechar = (char) _(from);
		//int ret;
		//ret = bpf_probe_read(userbuff, sizeof(userbuff), from);

		int ret;
		ret = bpf_probe_read_str(userbuff, sizeof(userbuff), from);

		char s[] = "full buffer %s\n";
		bpf_trace_printk(s, sizeof(s), userbuff);

			
		unsigned long sum = 0;
		char curr[3];
		unsigned long num = 0; // need initialization or verifier complains on strtol
		u64 base = 10;
		unsigned long elems = 3;

		for (int i = 0; i < UBUFFSIZE; i = i+3)
		{
			ret = bpf_probe_read_str(curr, 3, userbuff+i);
			
			if (curr != NULL)
			{
				int res = bpf_strtoul(curr, sizeof(curr), base, &num);
				if (res < 0)
				{
					return 1;
				}
			}

			char s3[] = "copyout char %s converted to %d\n";
			bpf_trace_printk(s3, sizeof(s3), curr, num);

			sum = sum + num;
		}
		

		unsigned long avg = sum/elems;
		
		char s4[] = "sum of numbers is %lu, avg is %lu\n";
		bpf_trace_printk(s4, sizeof(s4), sum, avg);

		bpf_map_update_elem(&result_map, &key, &avg, BPF_ANY);
		
		//doesn't work because then copyout bpf is called and overwrites this, until I have an integrated edit of the return value
		char mystring[] = "42";
		bpf_probe_write_user((void *) to, mystring, sizeof(mystring));

		//now this works, but then to is overwritten by the real function copyout so it is not returned to user
		//if only I could get return override, that highjacks execution T_T
		char s5[] = "now to is %s\n";
		bpf_trace_printk(s5, sizeof(s5), to);
		//bpf_probe_write_user((void *) from, (char*) &avg, sizeof(avg));

		bpf_my_printk();

		//unsigned long rc = 1;
		//bpf_override_return(ctx, rc);
		
	}

	return 0;
}

PROG(MIN_FUNC)(struct pt_regs *ctx)
{
    return 0;
}

PROG(MAX_FUNC)(struct pt_regs *ctx)
{
    return 0;
}


char _license[] SEC("license") = "GPL";
__u32 _version SEC("version") = LINUX_VERSION_CODE;