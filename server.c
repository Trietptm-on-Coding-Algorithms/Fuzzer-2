#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <mach/mach.h>
#include <stdlib.h>
#include <pthread.h>
#include "mach_exc.h"
#include "mach_excServer.c"

/* catch_mach_exception_raise */
 
kern_return_t catch_mach_exception_raise(
    mach_port_t exception_port, 
    mach_port_t thread_port,
    mach_port_t task_port,
    exception_type_t exception_type,
    mach_exception_data_t codes,
    mach_msg_type_number_t num_codes)
{
    /* exception_type is defined in exception_types.h                       */
 
    /* an exception may include a code and a sub-code.  num_codes specifies */
    /* the number of enties in the codes argument, between zero and two.    */
    /* codes[0] is the code, codes[1] is the sub-code.                      */
 
    if (exception_type == EXC_SOFTWARE && code[0] == EXC_SOFT_SIGNAL) {
     
        /* handling UNIX soft signal:                                       */
        /* this example clears SIGSTOP before resuming the process.         */
     
        if (codes[2] == SIGSTOP)
            codes[2] = 0;
 
        ptrace(PT_THUPDATE,
               target_pid,
               (void *)(uintptr_t)thread_port,
               codes[2]);
    }
     
    return KERN_SUCCESS;
}
 
/* catch_mach_exception_raise_state */
 
kern_return_t catch_mach_exception_raise_state(
    mach_port_t exception_port, 
    exception_type_t exception,
    const mach_exception_data_t code, 
    mach_msg_type_number_t codeCnt,
    int *flavor, 
    const thread_state_t old_state,
    mach_msg_type_number_t old_stateCnt, 
    thread_state_t new_state,
    mach_msg_type_number_t *new_stateCnt)
{
    /* not used because EXCEPTION_STATE is not specified in the call */
    /* to task_set_exception_ports, but referenced by mach_exc_server */
     
    return MACH_RCV_INVALID_TYPE;
}
 
/* catch_mach_exception_raise_state_identity */
 
kern_return_t catch_mach_exception_raise_state_identity(
    mach_port_t exception_port, 
    mach_port_t thread, 
    mach_port_t task,
    exception_type_t exception, 
    mach_exception_data_t code,
    mach_msg_type_number_t codeCnt, 
    int *flavor, 
    thread_state_t old_state,
    mach_msg_type_number_t old_stateCnt, 
    thread_state_t new_state,
    mach_msg_type_number_t *new_stateCnt)
{
    /* not used because EXCEPTION_STATE_IDENTITY is not specified in the   */
    /* call to task_set_exception_ports, but referenced by mach_exc_server */
     
    return MACH_RCV_INVALID_TYPE;
}

int main (int argc,char ** argv)
{
    int err;
    pthread_mutexattr_t attr;

    err = pthread_mutexattr_init(&attr); if (err) return err;
    err = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED); if (err) return err;

    pid_t pid = fork();
    int status;
    if (pid == 0)
    {
        printf ("[*] Child proces ...\n");
        FILE * f = fopen(argv[1],"rb");
        if (f == NULL)
        {
            return -2;
            puts ("Cannot open file specified\n");
        }
        int from,to = 0;
        sscanf(argv[2],"%x",&from);
        sscanf(argv[3],"%x",&to);
        if (from >= to)
        {
            puts ("R u out of your mind ? check your range of bytes within the file... \n");
            return -3;
        }
        int fileSize = to - from;
        void * mem = mmap (NULL,fileSize,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE,fileno(f),0);
        if (mem == MAP_FAILED)
        {
            puts ("[!] Cannot allocate memory for file");
            return -4;
        }
        printf ("[-] File mapped to virtual memory : [%p]\n",mem);

        int (*pFunc)(char * str) = (int(*)(char *))(mem+from);

        int ret = pFunc("AAAAA");
        printf ("Returned : %d\n",ret);
    }
    else 
    {
        int status;
        printf ("[*] Parent process ...\n");
        kern_return_t kret;
        kern_return_t kret_from_mach_msg;
        kern_return_t kret_from_catch_mach_exception_raise;
        mach_port_t task;
        mach_port_t target_exception_port;
        kret = task_for_pid (mach_task_self(),pid,&task);
        if (kret != KERN_SUCCESS)
        {
            printf ("task_for_pid() failed with message %s !\n",mach_error_string(kret));
        }
        else
        {
            printf ("SUCCESS\n");
        }
        //save the set of exception ports registered in the process 

        exception_mask_t       saved_masks[EXC_TYPES_COUNT];
        mach_port_t            saved_ports[EXC_TYPES_COUNT];
        exception_behavior_t   saved_behaviors[EXC_TYPES_COUNT];
        thread_state_flavor_t  saved_flavors[EXC_TYPES_COUNT];
        mach_msg_type_number_t saved_exception_types_count;

        task_get_exception_ports(task,
                        EXC_MASK_ALL,
                        saved_masks,
                        &saved_exception_types_count,
                        saved_ports,
                        saved_behaviors,
                        saved_flavors);

        //allocate and authorize a new port 

        mach_port_allocate(mach_task_self(),
                   MACH_PORT_RIGHT_RECEIVE,
                   &target_exception_port);

        mach_port_insert_right(mach_task_self(),
                       target_exception_port,
                       target_exception_port,
                       MACH_MSG_TYPE_MAKE_SEND);

        //register the exception port with the target process 

        task_set_exception_ports(task,
                         EXC_MASK_ALL,
                         target_exception_port,
                         EXCEPTION_DEFAULT | MACH_EXCEPTION_CODES,
                         THREAD_STATE_NONE);

        ptrace (PT_ATTACHEXC, pid,0,0);

        wait_for_exception:;
 
        char req[128];
        char rpl[128];            /* request and reply buffers */
 
        mach_msg((mach_msg_header_t *)req,  /* receive buffer */
         MACH_RCV_MSG,              /* receive message */
         0,                         /* size of send buffer */
         sizeof(req),               /* size of receive buffer */
         target_exception_port,     /* port to receive on */
         MACH_MSG_TIMEOUT_NONE,     /* wait indefinitely */
         MACH_PORT_NULL);           /* notify port, unused */
 
        /* suspend all threads in the process after an exception was received */
 
     
        task_suspend(task);

        if (kret_from_mach_msg == KERN_SUCCESS) 
        {
             /* mach_exc_server calls catch_mach_exception_raise */
     
            boolean_t message_parsed_correctly =
     
            mach_exc_server((mach_msg_header_t *)req,(mach_msg_header_t *)rpl);
     
            if (! message_parsed_correctly) 
            {
     
                 kret_from_catch_mach_exception_raise = ((mig_reply_error_t *)rpl)->RetCode;
            }
        }
 
        /* resume all threads in the process before replying to the exception */
 
        task_resume(task);
 
        /* reply to the exception */
 
        mach_msg_size_t send_sz = ((mach_msg_header_t *)rpl)->msgh_size;
 
        mach_msg((mach_msg_header_t *)rpl,  /* send buffer */
        MACH_SEND_MSG,             /* send message */
        send_sz,                   /* size of send buffer */
        0,                         /* size of receive buffer */
        MACH_PORT_NULL,            /* port to receive on */
        MACH_MSG_TIMEOUT_NONE,     /* wait indefinitely */
        MACH_PORT_NULL);           /* notify port, unused */
 
    /* target process is now running.  wait for a new exception */
 
    goto wait_for_exception;

    }
    
    return 0;
}