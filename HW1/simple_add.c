#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/linkage.h>

int n3;
asmlinkage long sys_simple_add(int n1, int n2, int *result) {
  printk(KERN_ALERT "Values are %d and %d\n", n1, n2) n3 = n1 + n2;
  copy_to_user(result, &n3, sizeof(int));
  printk(KERN_ALERT "Result is %d\n", n3);
  return 0;
}
