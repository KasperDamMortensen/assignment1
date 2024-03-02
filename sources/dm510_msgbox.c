#include "linux/unistd.h"
#include "linux/kernel.h"
#include "linux/uaccess.h"
#include "linux/slab.h"
unsigned long flags;
typedef struct _msg_t msg_t;
struct _msg_t{
  msg_t* previous;
  int length;
  char* message;
};
static msg_t *bottom = NULL;
static msg_t *top = NULL;
asmlinkage
int sys_dm510_msgbox_put( char *buffer, int length ) {
  if (buffer == NULL) {
    return -EINVAL;  //Invalid argument code
  }
  
  if (length < 0 || length > 128 ){ 
    return -EINVAL; 
  }
  if (!access_ok(buffer, length)){
    return -EFAULT;  // Bad address code
  }
  msg_t* msg = kmalloc(sizeof(msg_t), GFP_KERNEL);
  if (msg == NULL){
    return -ENOMEM;  // Out of memory code
  }
  msg->previous = NULL;
  msg->length = length;
  msg->message = kmalloc(length, GFP_KERNEL);
  if (msg->message == NULL){
    kfree(msg);
    return -ENOMEM;  
  }
  
  if (copy_from_user(msg->message, buffer, length) != 0) {
    kfree(msg);
    kfree(msg->message);
    return -EFAULT;  
  }
  local_irq_save(flags);
  if (bottom == NULL) {
    bottom = msg;
    top = msg;
  } else {
    /* not empty stack */
    msg->previous = top;
    top = msg;
  }
  local_irq_restore(flags);
  return 0;
}
asmlinkage
int sys_dm510_msgbox_get( char* buffer, int length ) {
  if (buffer == NULL) {
    return -EINVAL;
  }
  if (length < 0 || length > 128 ) { 
    return -EINVAL; 
  }
  if(!access_ok(buffer, length)){
    return -EFAULT;
  }
  if (top != NULL) {
    local_irq_save(flags);
    msg_t* msg = top;
    int mlength = msg->length;
    top = msg->previous;
    if (mlength > length){
      kfree(msg->message);
      kfree(msg);
      return -ENOBUFS;  // Buffer full
    }
    /* copy message */
    if (copy_to_user(buffer, msg->message, mlength) != 0) {
      kfree(msg->message);
      kfree(msg);
      return -EFAULT; 
    }
    /* free memory */
    kfree(msg->message);
    kfree(msg);
    local_irq_restore(flags);
    return mlength;
  }
  return -1;
}
