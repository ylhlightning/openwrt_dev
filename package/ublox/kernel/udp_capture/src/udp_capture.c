//’Hello World’ v2 netfilter hooks example
//For any packet, get the ip header and check the protocol field
//if the protocol number equal to UDP (17), log in var/log/messages
//default action of module to let all packets through
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/udp.h>
#include <linux/ip.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/mm.h>
#include <linux/slab.h>

#define buffer_len 32

#ifndef VM_RESERVED
# define  VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)
#endif

struct dentry  *file1;

static struct nf_hook_ops nfho;   //net filter hook option struct
struct sk_buff *sock_buff;
struct udphdr *udp_header;          //udp header struct (not used)
struct iphdr *ip_header;            //ip header struct
static int packet_number = 0;
  
typedef struct node
{
  unsigned int id;
  struct sk_buff sock_buff;
  struct node *prev;
  struct node *next;
}node_t;

node_t *udp_capture_chain_head;
node_t *udp_capture_chain_cur;

struct mmap_info {
  node_t *data;	/* the data */
  int reference;       /* how many times it is mmapped */
};

/* keep track of how many times it is mmapped */

void mmap_open(struct vm_area_struct *vma)
{
  struct mmap_info *info = (struct mmap_info *)vma->vm_private_data;
  info->reference++;
}

void mmap_close(struct vm_area_struct *vma)
{
  struct mmap_info *info = (struct mmap_info *)vma->vm_private_data;
  info->reference--;
}

/* nopage is called the first time a memory area is accessed which is not in memory,
 * it does the actual mapping between kernel and user space memory
 */
//struct page *mmap_nopage(struct vm_area_struct *vma, unsigned long address, int *type)	--changed
static int mmap_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
  struct page *page;
  struct mmap_info *info;
  info = (struct mmap_info *)vma->vm_private_data;
  if (!info->data) {
    printk("no data\n");
    return -1;	
  }

  /* get the page */
  page = virt_to_page(info->data);
	
  /* increment the reference count of this page */
  get_page(page);
  vmf->page = page;					//--changed
  return 0;
}

struct vm_operations_struct mmap_vm_ops = {
  .open =     mmap_open,
  .close =    mmap_close,
  .fault =    mmap_fault,
};

int my_mmap(struct file *filp, struct vm_area_struct *vma)
{
  vma->vm_ops = &mmap_vm_ops;
  vma->vm_flags |= VM_RESERVED;
  /* assign the file private data to the vm private data */
  vma->vm_private_data = filp->private_data;
  mmap_open(vma);
  return 0;
}

int my_close(struct inode *inode, struct file *filp)
{
  struct mmap_info *info = filp->private_data;
  /* obtain new memory */
  free_page((unsigned long)info->data);
  kfree(info);
  filp->private_data = NULL;
  return 0;
}

int my_open(struct inode *inode, struct file *filp)
{
  struct mmap_info *info = kmalloc(sizeof(struct mmap_info), GFP_KERNEL);
  /* obtain new memory */
  info->data = (node_t *)get_zeroed_page(GFP_KERNEL);
  memcpy(info->data, udp_capture_chain_head, buffer_len*sizeof(struct node));
  /* assign this info struct to the file */
  filp->private_data = info;
  return 0;
}

static const struct file_operations my_fops = {
  .open = my_open,
  .release = my_close,
  .mmap = my_mmap,
};

node_t *create_chain(unsigned int num)
{
  node_t *head = kmalloc(sizeof(struct node), GFP_KERNEL);
  node_t *cur = head;
  int i = 0;

  printk("Start to create chain.\n");

  for(i = 0; i < num; i++)
  {
    if(i == 0)
    {
      cur->next = kmalloc(sizeof(struct node), GFP_KERNEL);
      cur->id = i+1;
      cur->prev = NULL;
      printk("Node %d created with Node ID: %d.\n", i+1, cur->id);
      cur = cur->next;
    }
    else if(i == num -1)
    {
      cur->next = NULL;
      cur->id = i+1;
      printk("Last node pointed %d to NULL.\n", i+1);
    }
    else
    {
      cur->next = kmalloc(sizeof(struct node), GFP_KERNEL);
      cur->next->prev = cur;
      cur->id = i+1;
      printk("Node %d created with Node ID: %d.\n", i+1, cur->id);
      cur = cur->next;
    }
  }

  return head;
}

void print_chain(node_t * head)
{ 
  int i = 1;
  node_t *cur = head;;
  printk("Start to dump chain\n");
  while(cur->next != NULL)
  {
    printk("Node %d present with id:%d\n", i, cur->id);
    cur = cur->next;
    i++;
  }
}

unsigned int hook_func(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
  sock_buff = skb;
 
  ip_header = (struct iphdr *)skb_network_header(sock_buff);    //grab network header using accessor
       
  if(!sock_buff) { return NF_ACCEPT;}
 
  if (ip_header->protocol==17) {
    udp_header = (struct udphdr *)skb_transport_header(sock_buff);  //grab transport header
    printk(KERN_INFO "got udp packet \n");     //log we’ve got udp packet to /var/log/messages
    
    if(udp_capture_chain_cur->next != NULL)
    {
      memcpy(&udp_capture_chain_cur->sock_buff, sock_buff, sizeof(sock_buff)); 
      packet_number ++;
      printk(KERN_INFO "copy udp packet into chain position %d, %d buffer remains\n", udp_capture_chain_cur->id, buffer_len-packet_number);     //log we’ve got udp packet to /var/log/message
      udp_capture_chain_cur = udp_capture_chain_cur->next;
    }
    else
    {
      printk("totoal buffer %d full, pointer reset to head\n", buffer_len);
      udp_capture_chain_cur = udp_capture_chain_head;
      packet_number = 0;
    }
    return NF_DROP;
  }
  return NF_ACCEPT;
}
 
int init_module()
{
  nfho.hook = hook_func;
  nfho.hooknum = NF_INET_PRE_ROUTING;
  nfho.pf = PF_INET;
  nfho.priority = NF_IP_PRI_FIRST;
 
  udp_capture_chain_head = create_chain(buffer_len); 
  
  udp_capture_chain_cur = udp_capture_chain_head;
  
  print_chain(udp_capture_chain_head); 
 
  nf_register_hook(&nfho);

  file1 = debugfs_create_file("mmap_example", 0644, NULL, NULL, &my_fops);
       
  return 0;
}
 
void cleanup_module()
{
  nf_unregister_hook(&nfho);
  debugfs_remove(file1);    
}

MODULE_LICENSE("GPL");


