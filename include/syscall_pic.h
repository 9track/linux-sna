#define _syscall1_pic(type,name,type1,arg1)\
type name (type1 arg1)\
{\
   long __res;\
   __asm__ volatile (\
   "pushl %%ebx\n\t"\
   "movl %%eax,%%ebx\n\t"\
   "movl %1,%%eax\n\t"\
   "int $0x80\n\t"\
   "popl %%ebx"\
   : "=a" (__res)\
   : "i" (__NR_##name),"a" ((long)(arg1)));\
__syscall_return(type,__res);\
}

#define _syscall2_pic(type,name,type1,arg1,type2,arg2)\
type name (type1 arg1,type2 arg2)\
{\
   long __res;\
   __asm__ volatile (\
   "pushl %%ebx\n\t"\
   "movl %%eax,%%ebx\n\t"\
   "movl %1,%%eax\n\t"\
   "int $0x80\n\t"\
   "popl %%ebx"\
   : "=a" (__res)\
   : "i" (__NR_##name),"a" ((long)(arg1)),"c" ((long)(arg2)));\
__syscall_return(type,__res);\
}

#define _syscall4_pic(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4)\
type name (type1 arg1,type2 arg2,type3 arg3,type4 arg4)\
{\
   long __res;\
   __asm__ volatile (\
   "pushl %%ebx\n\t"\
   "movl %%eax,%%ebx\n\t"\
   "movl %1,%%eax\n\t"\
   "int $0x80\n\t"\
   "popl %%ebx"\
   : "=a" (__res)\
   : "i" (__NR_##name),"a" ((long)(arg1)),"c" ((long)(arg2)),\
   "d" ((long)(arg3)),"S" ((long)(arg4)));\
__syscall_return(type,__res);\
}

#define _syscall5_pic(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,\
      type5,arg5)\
type name (type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5)\
{\
   long __res;\
   __asm__ volatile (\
   "pushl %%ebx\n\t"\
   "movl %%eax,%%ebx\n\t"\
   "movl %1,%%eax\n\t"\
   "int $0x80\n\t"\
   "popl %%ebx"\
   : "=a" (__res)\
   : "i" (__NR_##name),"a" ((long)(arg1)),"c" ((long)(arg2)),\
   "d" ((long)(arg3)),"S" ((long)(arg4)),"D" ((long)(arg5)));\
__syscall_return(type,__res);\
}
