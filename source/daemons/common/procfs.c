/*___INFO__MARK_BEGIN__*/
/*************************************************************************
 * 
 *  The Contents of this file are made available subject to the terms of
 *  the Sun Industry Standards Source License Version 1.2
 * 
 *  Sun Microsystems Inc., March, 2001
 * 
 * 
 *  Sun Industry Standards Source License Version 1.2
 *  =================================================
 *  The contents of this file are subject to the Sun Industry Standards
 *  Source License Version 1.2 (the "License"); You may not use this file
 *  except in compliance with the License. You may obtain a copy of the
 *  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
 * 
 *  Software provided under this License is provided on an "AS IS" basis,
 *  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
 *  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
 *  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
 *  See the License for the specific provisions governing your rights and
 *  obligations concerning the Software.
 * 
 *   The Initial Developer of the Original Code is: Sun Microsystems, Inc.
 * 
 *   Copyright: 2001 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/* Portions of this code are Copyright (c) 2011 Univa Corporation. */
/*___INFO__MARK_END__*/
#if !defined(COMPILE_DC)

int verydummyprocfs;

#else

#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/signal.h>

#if !defined(AIX) && !defined(__CYGWIN__)
#include <sys/syscall.h>
#endif

#include <unistd.h>
#include <sys/times.h>
#include <sys/wait.h>
#if defined(FREEBSD) || defined(DARWIN)
#include <sys/time.h>
#endif
#include <sys/resource.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#if defined(ALPHA) 
#  include <sys/user.h>
#  include <sys/table.h>   
#  include <sys/procfs.h>   
#endif

#if defined(SOLARIS) 
#  include <sys/procfs.h>   
#endif

#if defined(LINUX)
#include "sgeobj/sge_proc.h"
#endif

#include "uti/sge_rmon.h"
#include "uti/sge_stdio.h"
#include "uti/sge_unistd.h"
#include "uti/sge_log.h"

#include "cull/cull.h"

#include "basis_types.h"
#include "sgedefs.h"
#include "exec_ifm.h"
#include "pdc.h"
#include "msg_sge.h"

#include "procfs.h"

#ifdef MONITOR_PDC
static bool monitor_pdc = true;
#else
static bool monitor_pdc = false;
#endif

#if defined(LINUX) || defined(ALPHA) || defined(SOLARIS)
#   if defined(SVR3)
#      define PROC_DIR "/debug"
#   else
#      define PROC_DIR "/proc"
#   endif
#endif

#ifdef LINUX
#define BIGLINE 1024
static int linux_proc_io(char *proc, uint64 *iochars);
static bool linux_read_status(char *proc, int time_stamp, lnk_link_t *job_list,
                              unsigned long *pid, unsigned long *utime,
                              unsigned long *stime, unsigned long *vsize);
#endif

/*-----------------------------------------------------------------------*/
#if defined(LINUX) || defined(ALPHA) || defined(SOLARIS)

static DIR *cwd;
static struct dirent *dent;

/* search in job list for the pid
   return the proc element */
static lnk_link_t *find_pid_in_jobs(pid_t pid, lnk_link_t *job_list)
{
   lnk_link_t *job, *proc = NULL;
   proc_elem_t *proc_elem = NULL;
   job_elem_t *job_elem = NULL;

   /*
    * try to find a matching job
    */
   for (job=job_list->next; job != job_list; job=job->next) {

      job_elem = LNK_DATA(job, job_elem_t, link);

      /*
       * try to find process in this jobs' proc list
       */

      for (proc=job_elem->procs.next; proc != &job_elem->procs;
               proc=proc->next) {

         proc_elem = LNK_DATA(proc, proc_elem_t, link);
         if (proc_elem->proc.pd_pid == pid)
            break; /* found it */
      }

      if (proc == &job_elem->procs) {
         /* end of procs list - no process found - try next job */
         proc = NULL;
      } else
         /* found a process */
         break;
   }

   return proc;
}


static void touch_time_stamp(const char *d_name, int time_stamp, lnk_link_t *job_list)
{
   pid_t pid;
   proc_elem_t *proc_elem;
   lnk_link_t *proc;

   DENTER(TOP_LAYER, "touch_time_stamp");

   sscanf(d_name, pid_t_fmt, &pid);
   if ((proc = find_pid_in_jobs(pid, job_list))) {
      proc_elem = LNK_DATA(proc, proc_elem_t, link);
      proc_elem->proc.pd_tstamp = time_stamp;
   }
   if (monitor_pdc) {
      if (proc)
         INFO((SGE_EVENT, "found job to process %s: set time stamp\n", d_name));
      else
         INFO((SGE_EVENT, "found no job to process %s\n", d_name));
   }
   DEXIT;
   return;
}

void procfs_kill_addgrpid(gid_t add_grp_id, int sig, tShepherd_trace shepherd_trace)
{
   char procnam[128];
   int i;
   int groups=0;
   u_long32 max_groups;
   gid_t *list;
#if defined(SOLARIS) || defined(ALPHA)
   int fd;
   prcred_t proc_cred;
#elif defined(LINUX)
   FILE *fp;
   char buffer[1024];
   uid_t uids[4] = {0,0,0,0};
   gid_t gids[4] = {0,0,0,0};
#endif

   DENTER(TOP_LAYER, "procfs_kill_addgrpid");

   /* quick return in case of invalid add. group id */
   if (add_grp_id == 0) {
      DEXIT;
      return;
   }

   max_groups = sge_sysconf(SGE_SYSCONF_NGROUPS_MAX);
   if (max_groups <= 0)
      if (shepherd_trace) {
         shepherd_trace(MSG_SGE_NGROUPS_MAXOSRECONFIGURATIONNECESSARY);
      }
/*
 * INSURE detects a WRITE_OVERFLOW when getgroups was invoked (LINUX).
 * Is this a bug in the kernel or in INSURE?
 */
#if __linux__
   list = (gid_t*) malloc(2*max_groups*sizeof(gid_t));
#else
   list = (gid_t*) malloc(max_groups*sizeof(gid_t));
#endif
   if (list == NULL)
      if (shepherd_trace) {
         shepherd_trace(MSG_SGE_PROCFSKILLADDGRPIDMALLOCFAILED);
      }

   pt_open();

   /* find next valid entry in procfs  */
   while ((dent = readdir(cwd))) {
      if (!dent->d_name)
         continue;
      if (!dent->d_name[0])
         continue;

      if (!strcmp(dent->d_name, "..") || !strcmp(dent->d_name, "."))
         continue;

      if (atoi(dent->d_name) == 0)
         continue;

#if defined(SOLARIS) || defined(ALPHA)
      snprintf(procnam, sizeof(procnam), "%s/%s", PROC_DIR, dent->d_name);
      if ((fd = open(procnam, O_RDONLY, 0)) == -1) {
         DPRINTF(("open(%s) failed: %s\n", procnam, strerror(errno)));
         continue;
      }
      /* get number of groups */
      if (ioctl(fd, PIOCCRED, &proc_cred) == -1) {
         close(fd);
         continue;
      }

      /* get list of supplementary groups */
      groups = proc_cred.pr_ngroups;
      if (ioctl(fd, PIOCGROUPS, list) == -1) {
         close(fd);
         continue;
      }
      close(fd);
#elif defined(LINUX)
      if (!strcmp(dent->d_name, "self"))
         continue;

      snprintf(procnam, sizeof(procnam), PROC_DIR "/%s/status", dent->d_name);
      if (!(fp = fopen(procnam, "r")))
         continue;
      /* get number of groups and current uids, gids
       * uids[0], gids[0] => UID and GID
       * uids[1], gids[1] => EUID and EGID
       * uids[2], gids[2] => SUID and SGID
       * uids[3], gids[3] => FSUID and FSGID
       */
      groups = 0;
      while (fgets(buffer, sizeof(buffer), fp)) {
         char *label = NULL;
         char *token = NULL;

         label = strtok(buffer, " \t\n");
         if (label) {
            if (!strcmp("Groups:", label)) {
               while ((token = strtok((char*) NULL, " \t\n"))) {
                  list[groups]=(gid_t) atol(token);
                  groups++;
               }
            } else if (!strcmp("Uid:", label)) {
               int i = 0;

               while ((i < 4) && (token = strtok((char*) NULL, " \t\n"))) {
                  uids[i]=(uid_t) atol(token);
                  i++;
               }
            } else if (!strcmp("Gid:", label)) {
               int i = 0;

               while ((i < 4) && (token = strtok((char*) NULL, " \t\n"))) {
                  gids[i]=(gid_t) atol(token);
                  i++;
               }
            }
         }
      }
      FCLOSE(fp);
FCLOSE_ERROR:
#endif  /* SOLARIS || ALPHA */

      /* send each process a signal which belongs to add_grg_id */
      for (i = 0; i < groups; i++) {
         if (list[i] == add_grp_id) {
            pid_t pid;
            pid = (pid_t) atol(dent->d_name);

#if defined(LINUX)
            /* if UID, GID, EUID and EGID == 0
             *  don't kill the process!!! - it could be the rpc.nfs-deamon
             */
            if (!(uids[0] == 0 && gids[0] == 0 &&
                  uids[1] == 0 && gids[1] == 0))
#elif defined(SOLARIS) || defined(ALPHA)
            if (!(proc_cred.pr_ruid == 0 && proc_cred.pr_rgid == 0 &&
                  proc_cred.pr_euid == 0 && proc_cred.pr_egid == 0))
#endif
              {

               if (shepherd_trace) {
                  char err_str[256];

                  snprintf(err_str, sizeof(err_str), MSG_SGE_KILLINGPIDXY_UI,
                           sge_u32c(pid), groups);
                  shepherd_trace(err_str);
               }

               kill(pid, sig);

            } else {
               if (shepherd_trace) {
                  char err_str[256];

                  snprintf(err_str, sizeof(err_str), MSG_SGE_DONOTKILLROOTPROCESSXY_UI,
                           sge_u32c(atol(dent->d_name)), groups);
                  shepherd_trace(err_str);
               }
            }

            break;
         }
      }
   }
   pt_close();
   sge_free(&list);
   DEXIT;
}

int pt_open(void)
{
   cwd = opendir(PROC_DIR);
   return !cwd;
}
void pt_close(void)
{
   closedir(cwd);
}

int pt_dispatch_proc_to_job(lnk_link_t *job_list, int time_stamp,
                            time_t last_time) {
   int fd = -1;
#if defined(LINUX)
   lListElem *pr = NULL;
   SGE_STRUCT_STAT fst;
   unsigned long utime, stime, vsize, pid;
   int pos_pid = lGetPosInDescr(PRO_Type, PRO_pid);
   int pos_utime = lGetPosInDescr(PRO_Type, PRO_utime);
   int pos_stime = lGetPosInDescr(PRO_Type, PRO_stime);
   int pos_vsize = lGetPosInDescr(PRO_Type, PRO_vsize);
   int pos_groups = lGetPosInDescr(PRO_Type, PRO_groups);
   int pos_rel = lGetPosInDescr(PRO_Type, PRO_rel);
   int pos_run = lGetPosInDescr(PRO_Type, PRO_run);
   int pos_io = lGetPosInDescr(PRO_Type, PRO_io);
   int pos_group = lGetPosInDescr(GR_Type, GR_group);
#else
   char procnam[128];
   prstatus_t pr;
   prpsinfo_t pri;
#endif  /* LINUX */

#if defined(SOLARIS) || defined(ALPHA)   
   prcred_t proc_cred;
   int ret;
#endif

   u_long32 max_groups;
   gid_t *list;
   int groups=0;
   int pid_tmp;

   proc_elem_t *proc_elem = NULL;
   job_elem_t *job_elem = NULL;
   lnk_link_t *curr;
   double old_time = 0;
   uint64 old_vmem = 0;

   DENTER(TOP_LAYER, "pt_dispatch_proc_to_job");

   max_groups = sge_sysconf(SGE_SYSCONF_NGROUPS_MAX);
   if (max_groups <= 0) {
      ERROR((SGE_EVENT, SFNMAX, MSG_SGE_NGROUPS_MAXOSRECONFIGURATIONNECESSARY));
      DEXIT;
      return 1;  
   }   

   list = (gid_t*) malloc(max_groups*sizeof(gid_t));
   if (list == NULL) {
      ERROR((SGE_EVENT, SFNMAX, MSG_SGE_PTDISPATCHPROCTOJOBMALLOCFAILED));
      DEXIT;
      return 1;
   }

   /* find next valid entry in procfs */ 
   while ((dent = readdir(cwd))) {
      char *pidname;

      if (!dent->d_name)
         continue;
      if (!dent->d_name[0])
         continue;

      if (!strcmp(dent->d_name, "..") || !strcmp(dent->d_name, "."))
         continue;

      if (dent->d_name[0] == '.') /* ?? */
          pidname = &dent->d_name[1];
      else
          pidname = dent->d_name;

      if (atoi(pidname) == 0)
         continue;

#if defined(LINUX)
      /* check only processes which belong to a GE job */
      if ((pr = get_pr(atoi(pidname))) != NULL) {
         /* set process as still running */
         lSetPosBool(pr, pos_run, true);
         if (lGetPosBool(pr, pos_rel) != true) {
            continue;
         }
      }

      if (!(linux_read_status(dent->d_name, time_stamp, job_list,
                              &pid, &utime, &stime, &vsize) != 0))
         continue;

      if (pr == NULL) {
         pr = lCreateElem(PRO_Type);
         lSetPosUlong(pr, pos_pid, pid);
         lSetPosBool(pr, pos_rel, false);
         append_pr(pr);
      }
         
      lSetPosUlong(pr, pos_utime, utime);
      lSetPosUlong(pr, pos_stime, stime);
      lSetPosUlong64(pr, pos_vsize, vsize);
         
      /* mark this proc as running */
      lSetPosBool(pr, pos_run, true);

      /* 
       * get number of groups; 
       * get list of supplementary groups 
       */
      {
         char procnam[256];
         lList *groupTable = lGetPosList(pr, pos_groups);

         snprintf(procnam, sizeof(procnam), PROC_DIR "/%s/status", dent->d_name);
         if (SGE_STAT(procnam, &fst) != 0) {
            if (errno != ENOENT) {
               if (monitor_pdc) {
                  INFO((SGE_EVENT, "could not stat %s: %s\n", procnam,
                        strerror(errno)));
               }
               touch_time_stamp(dent->d_name, time_stamp, job_list);
            }
            continue;
         }

         groups = 0;
         if (fst.st_mtime < last_time && groupTable != NULL) {
            lListElem *group;

            for_each(group, groupTable) {
               list[groups] = lGetPosUlong(group, pos_group);
               groups++;
            }
         } else {
            char buf[1024];
            FILE* f = (FILE*) NULL;

            if (!(f = fopen(procnam, "r"))) {
               continue;
            }
            /* save groups also in the table */
            groupTable = lCreateList("groupTable", GR_Type);
            while (fgets(buf, sizeof(buf), f)) {
               if (strcmp("Groups:", strtok(buf, "\t"))==0) {
                  char *token;
                  
                  while ((token=strtok((char*) NULL, " "))) {
                     lListElem *gr = lCreateElem(GR_Type);
                     long group = atol(token);
                     list[groups] = group;
                     lSetPosUlong(gr, pos_group, group);
                     lAppendElem(groupTable, gr);
                     groups++;
                  }
                  break;
               }
            }
            lSetPosList(pr, pos_groups, groupTable);
            fclose(f);
         }
      } 

#  elif defined(SOLARIS) || defined(ALPHA)

      snprintf(procnam, sizeof(procnam), "%s/%s", PROC_DIR, dent->d_name);
      if ((fd = open(procnam, O_RDONLY, 0)) == -1) {
         if (errno != ENOENT) {
            if (monitor_pdc) {
               if (errno == EACCES)
                  INFO((SGE_EVENT, "(uid:"gid_t_fmt" euid:"gid_t_fmt
                        ") could not open %s: %s\n",
                        getuid(), geteuid(), procnam, strerror(errno)));
               else
                  INFO((SGE_EVENT, "could not open %s: %s\n", procnam,
                        strerror(errno)));
            }
            touch_time_stamp(dent->d_name, time_stamp, job_list);
         }
         continue;
      }

      /**
       ** get a list of supplementary group ids to decide
       ** whether this process will be needed;
       ** read also prstatus
       **/
      
      /* 
       * get prstatus 
       */
      if (ioctl(fd, PIOCSTATUS, &pr)==-1) {
         close(fd);
         if (errno != ENOENT) {
            if (monitor_pdc)
               INFO((SGE_EVENT, "could not ioctl(PIOCSTATUS) %s: %s\n",
                     procnam, strerror(errno)));
            touch_time_stamp(dent->d_name, time_stamp, job_list);
         }
         continue;
      }
                                    
      /* 
       * get number of groups 
       */
      ret=ioctl(fd, PIOCCRED, &proc_cred);
      if (ret < 0) {
         close(fd);
         if (errno != ENOENT) {
            if (monitor_pdc)
               INFO((SGE_EVENT, "could not ioctl(PIOCCRED) %s: %s\n", procnam,
                     strerror(errno)));
            touch_time_stamp(dent->d_name, time_stamp, job_list);
         }
         continue;
      }
      
      /* 
       * get list of supplementary groups 
       */
      groups = proc_cred.pr_ngroups;
      ret=ioctl(fd, PIOCGROUPS, list);
      if (ret<0) {
         close(fd);
         if (errno != ENOENT) {
            if (monitor_pdc)
               INFO((SGE_EVENT, "could not ioctl(PIOCCRED) %s: %s\n", procnam,
                     strerror(errno)));
            touch_time_stamp(dent->d_name, time_stamp, job_list);
         }
         continue;
      }

#  endif  /* LINUX */

      /* 
       * try to find a matching job 
       */
      for (curr=job_list->next; curr != job_list; curr=curr->next) {
         int found_it = 0;
         int group;
         
         job_elem = LNK_DATA(curr, job_elem_t, link);
         for (group=0; !found_it && group<groups; group++) {
            if (job_elem->job.jd_jid == list[group]) {
#if defined(LINUX)
               /* mark this process as relevant */
               lSetPosBool(pr, pos_rel, true);
#endif
               found_it = 1;
            }
         }
         if (found_it)
            break;
      }

      if (curr == job_list) { /* this is not a traced process */ 
         close(fd);
         continue;
      }

      /* we always read only one entry per function call
         the while loop is needed to read next one */
      break;
   } /* while */

   sge_free(&list);

   if (!dent) {/* visited all files in procfs */
      clean_procList();
      DEXIT;
      return 1;
   }
   /* 
    * try to find process in this jobs' proc list 
    */

#if defined(LINUX)
   pid_tmp = lGetPosUlong(pr, pos_pid);
#else
   pid_tmp = pr.pr_pid;
#endif
   for (curr=job_elem->procs.next; curr != &job_elem->procs; 
            curr=curr->next) {
      proc_elem = LNK_DATA(curr, proc_elem_t, link);
      
      if (proc_elem->proc.pd_pid == pid_tmp)
         break;
   }

   if (curr == &job_elem->procs) { 
      /* new process, add a proc element into jobs proc list */
      if (!(proc_elem=(proc_elem_t *)malloc(sizeof(proc_elem_t)))) {
         if (fd >= 0)
            close(fd);
         DEXIT;
         return 0;
      }
      memset(proc_elem, 0, sizeof(proc_elem_t));
      proc_elem->proc.pd_length = sizeof(psProc_t);
      proc_elem->proc.pd_state  = 1; /* active */
      LNK_ADD(job_elem->procs.prev, &proc_elem->link);
      job_elem->job.jd_proccount++;

      if (monitor_pdc) {
         double utime, stime;
#if defined(LINUX)
         utime = ((double)lGetPosUlong(pr, pos_utime))/sysconf(_SC_CLK_TCK);
	 stime = ((double)lGetPosUlong(pr, pos_stime))/sysconf(_SC_CLK_TCK);

         INFO((SGE_EVENT, "new process "sge_u32" for job "pid_t_fmt
               " (utime = %f stime = %f)\n",
               lGetPosUlong(pr, pos_pid), job_elem->job.jd_jid, utime, stime)); 
#else
         utime = pr.pr_utime.tv_sec + pr.pr_utime.tv_nsec*1E-9;
         stime = pr.pr_stime.tv_sec + pr.pr_stime.tv_nsec*1E-9;

         INFO((SGE_EVENT, "new process "pid_t_fmt" for job "pid_t_fmt
               " (utime = %f stime = %f)\n",
               pr.pr_pid, job_elem->job.jd_jid, utime, stime)); 
#endif  /* LINUX */
      }
   } else {
      /* save previous usage data - needed to build delta usage */
      old_time = proc_elem->proc.pd_utime + proc_elem->proc.pd_stime;
      old_vmem  = proc_elem->vmem;
   }

   proc_elem->proc.pd_tstamp = time_stamp;

#if defined(LINUX)
   proc_elem->proc.pd_pid = lGetPosUlong(pr, pos_pid);
   proc_elem->proc.pd_utime  = ((double)lGetPosUlong(pr, pos_utime)) /
     sysconf(_SC_CLK_TCK);
   proc_elem->proc.pd_stime  = ((double)lGetPosUlong(pr, pos_stime)) /
     sysconf(_SC_CLK_TCK);
   /* could retrieve uid/gid using stat() on stat file */
   proc_elem->vmem           = lGetPosUlong64(pr, pos_vsize);

   /*
    * I/O accounting
    */
   proc_elem->delta_chars = 0UL;
   {
      uint64 new_iochars = 0UL;

      if (linux_proc_io(dent->d_name, &new_iochars) == 0)
         lSetPosUlong(pr, pos_io, new_iochars);
      else
         new_iochars = lGetPosUlong(pr, pos_io);
      /*
       *  Update process I/O info
       */
      if (new_iochars > 0UL)
      {
         uint64 old_iochars = proc_elem->iochars;

         if (new_iochars > old_iochars)
         {
            proc_elem->delta_chars = (new_iochars - old_iochars);
            proc_elem->iochars = new_iochars;
         }
      }
   }
#else  /* !LINUX */
   proc_elem->proc.pd_pid    = pr.pr_pid;
   proc_elem->proc.pd_utime  = pr.pr_utime.tv_sec + pr.pr_utime.tv_nsec*1E-9;
   proc_elem->proc.pd_stime  = pr.pr_stime.tv_sec + pr.pr_stime.tv_nsec*1E-9;
    
   /* Don't care if this part fails */
   if (ioctl(fd, PIOCPSINFO, &pri) != -1) {
      proc_elem->proc.pd_uid    = pri.pr_uid;
      proc_elem->proc.pd_gid    = pri.pr_gid;
      proc_elem->vmem           = pri.pr_size * pagesize;
      proc_elem->rss            = pri.pr_rssize * pagesize;
      proc_elem->proc.pd_pstart = pri.pr_start.tv_sec + pri.pr_start.tv_nsec*1E-9;
   }
#endif  /* LINUX */

   proc_elem->mem = 
         ((proc_elem->proc.pd_stime + proc_elem->proc.pd_utime) - old_time) * 
         (( old_vmem + proc_elem->vmem)/2);

#if defined(ALPHA)
#define BLOCKSIZE 512
   {
      struct user ua;
      uint64 old_ru_ioblock = proc_elem->ru_ioblock;

      /* need to do a table(2) call for each process to retrieve io usage data */   
      /* get user area stuff */
      if (table(TBL_UAREA, proc_elem->proc.pd_pid, (char *)&ua, 1, sizeof ua) == 1) {
         proc_elem->ru_ioblock = (uint64)(ua.u_ru.ru_inblock + ua.u_ru.ru_oublock);
         proc_elem->delta_chars = (proc_elem->ru_ioblock - old_ru_ioblock)* BLOCKSIZE;
      }
   }
#endif  /* ALPHA */

   close(fd);
   DEXIT;
   return 0;
}
#endif  /* LINUX || ALPHA || SOLARIS */

#ifdef LINUX
/* Return in IOCHARS the number of characters of i/o (read and write)
   for process PROC.  Function value is zero iff the procfs io file is
   found (which needs CONFIG_TASK_IO_ACCOUNTING=y in the Linux
   config.)  */
static int linux_proc_io(char *proc, uint64 *iochars)
{
   char procnam[256];
   SGE_STRUCT_STAT fst;

   snprintf(procnam, sizeof(procnam), PROC_DIR "/%s/io", proc);
   if (SGE_STAT(procnam, &fst) != 0)
      return 1;
   FILE *fd;

   if ((fd = fopen(procnam, "r"))) {
      char buf[1024];

      while (fgets(buf, sizeof(buf), fd)) {
         char *label = strtok(buf, " \t\n");
         char *token = strtok((char*) NULL, " \t\n");

         if (label && token)
            if (!strcmp("rchar:", label) || !strcmp("wchar:", label)) {
               unsigned long long nchar = 0UL;
               if (sscanf(token, "%Lu", &nchar) == 1)
                  *iochars += (uint64) nchar;
            }
      }
      fclose(fd);
      return 0;
   }
   return 1;
}

/* True if we can read the swap value from /proc/self/status.
   Not thread-safe; initially called by psStartCollector. */
bool swap_in_status (void)
{
   FILE *fp;
   char procnam[256];
   static bool ret = false, called = false;

   if (called) return ret;
   called = true;
   if ((fp = fopen(PROC_DIR "/self/status", "r"))) {
      while (fgets(procnam, sizeof procnam, fp))
         if (strncmp(procnam, "VmSwap:", 7) == 0) {
            ret = true;
            break;
         }
      fclose(fp);
   }
   return ret;
}

/* True if we can read the swap value from /proc/self/smaps.
   smaps present since Linux 2.6.14 iff CONFIG_MMU.
   Not thread-safe; initially called by psStartCollector. */
bool swap_in_smaps(void)
{
   FILE *fp;
   char procnam[256];
   static bool ret = false, called = false;

   if (called) return ret;
   called = true;
   if ((fp = fopen(PROC_DIR "/self/smaps", "r"))) {
      while (fgets(procnam, sizeof procnam, fp))
         if (strncmp(procnam, "Swap:", 5) == 0) {
            ret = true;
            break;
         }
      fclose(fp);
   }
   return ret;
}

/* True if we can read PSS from /proc/self/smaps.
   This is the process' proportional share of RSS.  PSS+swap is the most
   useful memory consumption measure.
   Not clear when PSS was introduced -- in Debian 2.6.32 but not RHEL 5.
   [Why isn't the PSS total in status?]
   Not thread-safe; initially called by psStartCollector. */
bool pss_in_smaps(void)
{
   FILE *fp;
   char procnam[256];
   static bool ret = false, called = false;

   if (called) return ret;
   called = true;
   if ((fp = fopen(PROC_DIR "/self/smaps", "r"))) {
      while (fgets(procnam, sizeof procnam, fp))
         if (strncmp(procnam, "Pss:", 4) == 0) {
            ret = true;
            break;
         }
      fclose(fp);
   }
   return ret;
}

/* Read data from the status file in /proc for process named PROC,
   updating the timestamp for in JOB_LIST.  Return process' PID, user
   time (UTIME), system time (STIME) and memory size (VMSIZE).  */
static bool
linux_read_status(char *proc, int time_stamp, lnk_link_t *job_list,
                  unsigned long *pid, unsigned long *utime,
                  unsigned long *stime, unsigned long *vmsize)
{
   char procnam[256], buffer[BIGLINE];
/*    SGE_STRUCT_STAT fst; */
   int fd, ret;

   DENTER(TOP_LAYER, "linux_read_status");
   snprintf(procnam, sizeof(procnam), PROC_DIR "/%s/stat", proc);
   errno = 0;
   /*
   if (SGE_STAT(procnam, &fst)) {
      if (errno != ENOENT) {
         if (monitor_pdc)
            INFO((SGE_EVENT, "could not stat %s: %s\n", procnam,
                  strerror(errno)));
         touch_time_stamp(proc, time_stamp, job_list);
      }
      DRETURN(false);
   }
   */
   /* TODO (SH): This does not work with Linux 2.6. I'm looking for a workaround.
    * If the stat file was not changed since our last parsing there is no need to do it again
    */
   /*if (pr == NULL || fst.st_mtime > last_time) */
   {
      errno = 0;
      if ((fd = open(procnam, O_RDONLY, 0)) == -1) {
         if (errno != ENOENT) {
            if (monitor_pdc) {
               if (errno == EACCES)
                  INFO((SGE_EVENT,
                        "(uid:"gid_t_fmt" euid:"gid_t_fmt
                        ") could not open %s: %s\n",
                        getuid(), geteuid(), procnam, strerror(errno)));
               else
                  INFO((SGE_EVENT, "could not open %s: %s\n", procnam,
                        strerror(errno)));
            }
            touch_time_stamp(proc, time_stamp, job_list);
         }
         DRETURN(false);
      }
      if ((ret = read(fd, buffer, BIGLINE-1))<=0) {
         close(fd);
         if (ret == -1 && errno != ENOENT) {
            if (monitor_pdc)
               INFO((SGE_EVENT, "could not read %s: %s\n", procnam,
                     strerror(errno)));
            touch_time_stamp(proc, time_stamp, job_list);
         }
         DRETURN(false);
      }
      close(fd);
      buffer[BIGLINE-1] = '\0';

      /* data from stat file */
      ret = sscanf(buffer, "%lu %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u "
                   "%*u %lu %lu %*d %*d %*d %*d %*d %*d %*u %lu",
                   pid, utime, stime, vmsize);
      if (ret != 4)
         DRETURN(false);
      /* Get more accurate memory consumption than VMsize if possible.  */
      {
         unsigned long vvmsize = 0, value = 0;
         FILE *fp;
         char key[20]; /* Size must match width in sscanf */

         errno = 0;
         /* Ideally, use PSS for best accuracy.  */
         if (pss_in_smaps()) {
            snprintf(procnam, sizeof procnam, PROC_DIR "/%s/smaps", proc);
            if ((fp = fopen(procnam, "r"))) {
               while(fgets(procnam, sizeof procnam, fp))
                  if (sscanf(procnam, "%20s %lu", key, &value) == 2)
                     if (strcmp(key, "Swap:") == 0
                         || (strcmp(key, "Pss:") == 0))
                        vvmsize += value * 1024;
               fclose(fp);
            } else if (monitor_pdc)
               INFO((SGE_EVENT, "could not read %s: %s\n", procnam,
                     strerror(errno)));
         } else if (swap_in_status()) {
            /* Slightly quicker if we have it -- maybe not worth bothering.  */
            snprintf(procnam, sizeof procnam, PROC_DIR "/%s/status", proc);
            if ((fp = fopen(procnam, "r"))) {
               bool gotone = false;
               while(fgets(procnam, sizeof procnam, fp)) {
                  if (sscanf(procnam, "%20s %lu", key, &value) == 2)
                     if (strncmp(procnam, "VmRSS:", 6) == 0
                         || strncmp(procnam, "VmSwap:", 7) == 0) {
                        vvmsize += value * 1024;
                        if (gotone) break;
                        gotone = true;
                     }
               }
               fclose(fp);
            }
            else if (monitor_pdc)
               INFO((SGE_EVENT, "could not read %s: %s\n", procnam,
                     strerror(errno)));
         } else if (swap_in_smaps()) {
            snprintf(procnam, sizeof procnam, PROC_DIR "/%s/smaps", proc);
            if ((fp = fopen(procnam, "r"))) {
               while(fgets(procnam, sizeof procnam, fp))
                  if (sscanf(procnam, "%20s %lu", key, &value) == 2)
                     if (strcmp(key, "Swap:") == 0
                         || (strcmp(key, "Rss:") == 0))
                        vvmsize += value * 1024;
               fclose(fp);
            } else if (monitor_pdc)
               INFO((SGE_EVENT, "could not read %s: %s\n", procnam,
                     strerror(errno)));
         }
         if (vvmsize > 0)
            *vmsize = vvmsize;
      }
   }
   DRETURN(true);
}

#endif  /* LINUX */

void
init_procfs(void)
{
#ifdef LINUX
   /* Initialize the tests.  */
   (void) pss_in_smaps();
   (void) swap_in_smaps();
   (void) swap_in_status();
#endif
}

#endif /* (!COMPILE_DC) */
