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
/*___INFO__MARK_END__*/
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h>
#include <fcntl.h>


#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/ToggleB.h>

#include <Xmt/Xmt.h>
#include <Xmt/Hash.h>
#include <Xmt/Icon.h>
#include <Xmt/Create.h>
#include <Xmt/Chooser.h>
#include <Xmt/Layout.h>
#include <Xmt/Dialog.h>
#include <Xmt/Dialogs.h>
#include <Xmt/InputField.h>
#include <Xmt/MsgLine.h>

/*----------------------------------------------------------------------------*/
#include "msg_common.h"
#include "msg_clients_common.h"
#include "msg_qmon.h"

#include "sge_unistd.h"
#include "sge_prog.h"
#include "sge_all_listsL.h"
#include "sge_userset.h"
#include "AskForTime.h"
#include "Matrix.h"
#include "IconList.h"
#include "symbols.h"
#include "parse_qsub.h"
#include "sge_time.h"
#include "parse_job_cull.h"
#include "unparse_job_cull.h"
#include "read_defaults.h"
#include "write_job_defaults.h"
#include "qmon_proto.h"
#include "qmon_rmon.h"
#include "qmon_appres.h"
#include "qmon_util.h"
#include "qmon_cull.h"
#include "qmon_arsub.h"
#include "qmon_comm.h"
#include "qmon_request.h"
#include "qmon_globals.h"
#include "qmon_widgets.h"
#include "qmon_quarks.h"
#include "qmon_timer.h"
#include "qmon_message.h"
#include "qmon_ar.h"
#include "qmon_init.h"
#include "sge_feature.h"
#include "sge_afsutil.h"
#include "sge_range.h"
#include "sge_path_alias.h"
#include "qm_name.h"
#include "sge_security.h" 
#include "sge_job.h"
#include "sge_stdlib.h"
#include "sge_io.h"
#include "sge_var.h"
#include "sge_answer.h"
#include "sge_ulong.h"
#include "sge_ja_task.h"
#include "sge_string.h"
#include "sge_gdi_ctx.h"
#include "uti/sge_string.h"
#include "uti/setup_path.h"
#include "sgeobj/sge_advance_reservation.h"


extern sge_gdi_ctx_class_t *ctx;

extern char **environ;

/*-------------------------------------------------------------------------*/

typedef struct _tARSMEntry {
   String   ar_name;
   String   ckpt_obj;
   String   cell;
   String   account_string;
   String   pe;
   lList    *mail_list;             /* MR_Type */
   lList    *resource_list;     
   lList    *queue_list;       /* QR_Type */
   int      mail_options;
   Cardinal start_time;
   Cardinal end_time;
   Cardinal duration;
   int      now;
   int      verify_mode;
   int      handle_hard_error;
} tARSMEntry;

   
XtResource arsm_resources[] = {
   { "ar_name", "ar_name", XtRString,
      sizeof(String),
      XtOffsetOf(tARSMEntry, ar_name),
      XtRImmediate, NULL },

   { "ckpt_obj", "ckpt_obj", XtRString,
      sizeof(String),
      XtOffsetOf(tARSMEntry, ckpt_obj),
      XtRImmediate, NULL },

   { "account_string", "account_string", XtRString,
      sizeof(String),
      XtOffsetOf(tARSMEntry, account_string),
      XtRImmediate, NULL },

   { "start_time", "start_time", XtRCardinal,
      sizeof(Cardinal), XtOffsetOf(tARSMEntry, start_time),
      XtRImmediate, NULL },

   { "end_time", "end_time", XtRCardinal,
      sizeof(Cardinal), XtOffsetOf(tARSMEntry, end_time),
      XtRImmediate, NULL },

   { "duration", "duration", XtRCardinal,
      sizeof(Cardinal), XtOffsetOf(tARSMEntry, duration),
      XtRImmediate, NULL },

   { "pe", "pe", XtRString,
      sizeof(String),
      XtOffsetOf(tARSMEntry, pe),
      XtRImmediate, NULL },

   { "mail_options", "mail_options", XtRInt,
      sizeof(int), XtOffsetOf(tARSMEntry, mail_options),
      XtRImmediate, NULL }, 

   { "mail_list", "mail_list", QmonRMR_Type,
      sizeof(lList*), XtOffsetOf(tARSMEntry, mail_list),
      XtRImmediate, NULL },

   { "queue_list", "queue_list", QmonRQR_Type,
      sizeof(lList*), XtOffsetOf(tARSMEntry, queue_list),
      XtRImmediate, NULL },

   { "verify_mode", "verify_mode", XtRInt,
      sizeof(int), XtOffsetOf(tARSMEntry, verify_mode),
      XtRImmediate, NULL },

   { "handle_hard_error", "handle_hard_error", XtRInt,
      sizeof(int), XtOffsetOf(tARSMEntry, handle_hard_error),
      XtRImmediate, NULL }

};


/*-------------------------------------------------------------------------*/
static Widget qmonARSubCreate(Widget parent);
static void qmonARSubPopdown(Widget w, XtPointer cld, XtPointer cad);
static void qmonARSubARSub(Widget w, XtPointer cld, XtPointer cad);
static void qmonARSubMailList(Widget w, XtPointer cld, XtPointer cad);
static void qmonARSubCancel(Widget w, XtPointer cld, XtPointer cad);
static void qmonARSubOkay(Widget w, XtPointer cld, XtPointer cad);
static void qmonARSubReset(Widget w, XtPointer cld, XtPointer cad);
static void qmonARSubClear(Widget w, XtPointer cld, XtPointer cad);
static void qmonARSubDelete(Widget w, XtPointer cld, XtPointer cad);
static void qmonARSubMailInput(Widget w, XtPointer cld, XtPointer cad);
static void qmonARSubCreateDialogs(Widget w);
static void qmonARSubStartEndTime(Widget w, XtPointer cld, XtPointer cad);
static void qmonARSubDuration(Widget w, XtPointer cld, XtPointer cad);
static void qmonARSubAskForPE(Widget w, XtPointer cld, XtPointer cad);
static void qmonARSubAskForCkpt(Widget w, XtPointer cld, XtPointer cad);
static void qmonARSubCheckARName(Widget w, XtPointer cld, XtPointer cad);
static void qmonARSubToggleDuration(Widget w, XtPointer cld, XtPointer cad);

static Boolean qmonCullToSM(lListElem *jep, tARSMEntry *data);
static Boolean qmonSMToCull(tARSMEntry *data, lListElem *jep, int save);
static void qmonFreeARSMData(tARSMEntry *data);
static void qmonInitARSMData(tARSMEntry *data);
static u_long32 ConvertMailOptions(int mail_options);
static int MailOptionsToDialog(u_long32 mail_options);
static void qmonARSubChangeResourcesPixmap(void); 
static void qmonARSubSetSensitive(int mode);
static void qmonPopupARSubResource(Widget w, XtPointer cld, XtPointer cad);
static void qmonCreateARSubResource(Widget parent, XtPointer cld);
static void qmonARResourceOK(Widget w, XtPointer cld, XtPointer cad);
static void qmonARResourceCancel(Widget w, XtPointer cld, XtPointer cad);
static void qmonARResourceEditResource(Widget w, XtPointer cld, XtPointer cad);
static void qmonARResourceClearResources(Widget w, XtPointer cld, XtPointer cad);
static void qmonARResourceRemoveResource(Widget w, XtPointer cld, XtPointer cad);
static void qmonARResourceSetResources(Widget w, XtPointer cld, XtPointer cad);
/*-------------------------------------------------------------------------*/

static Widget qmon_arsub = 0;
static Widget arsub_layout = 0;
static Widget arsub_name = 0;
static Widget arsub_start_time = 0;
static Widget arsub_start_timePB = 0;
static Widget arsub_end_time = 0;
static Widget arsub_end_timePB = 0;
static Widget arsub_ckpt_obj = 0;
static Widget arsub_ckpt_objPB = 0;
static Widget arsub_duration_row = 0;
static Widget arsub_duration = 0;
static Widget arsub_durationPB = 0;
static Widget arsub_pe = 0;
static Widget arsub_pePB = 0;
static Widget arsub_resources = 0;
static Widget arsub_mail = 0;
static Widget arsub_mail_user = 0;
static Widget arsub_mail_userPB = 0;
static Widget arsub_main_link = 0; 
static Widget arsub_message = 0;
static Widget arsub_duration_toggle = 0;

static Widget arsub_arsub = 0;
static Widget arsub_done = 0;
static Widget arsub_clear = 0;
static Widget arresource = 0;
static Widget arresource_ar = 0;
static Widget arresource_sr = 0;

/* subdialogs */
static Widget armail_list_w = 0;

static tARSubMode arsub_mode_data = {ARSUB_NORMAL, ARSUB_NORMAL|ARSUB_SCRIPT, 0};
static tARSMEntry ARSMData; 
/*
** resources
*/
static lList *arresource_resources = NULL;

/*-------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------*/
/* PUBLIC                                                                  */
/*-------------------------------------------------------------------------*/
void qmonARSubPopup(Widget w, XtPointer cld, XtPointer cad)
{
   tARSubMode *data = (tARSubMode *)cld;
   XmString xtitle = NULL;
   char buf[128];
   lList *alp = NULL;

   DENTER(GUI_LAYER, "qmonARSubPopup");
   
   /* set busy cursor */
   XmtDisplayBusyCursor(w);

   qmonMirrorMultiAnswer(AR_T | USERSET_T | PROJECT_T | PE_T | CKPT_T, &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      lFreeList(&alp);
      /* set default cursor */
      XmtDisplayDefaultCursor(w);
      DEXIT;
      return;
   }
   
   if (!qmon_arsub) {
      /*
      ** create the main dialog, if it doesn't exist
      */
      qmon_arsub = qmonARSubCreate(AppShell);

#ifdef FIXME
      /* 
      ** set the close button callback 
      ** set the icon and iconName
      */
      XmtCreatePixmapIcon(qmon_arsub, qmonGetIcon("toolbar_arsub"), None);
      XtVaSetValues(qmon_arsub, XtNiconName, "qmon:ARSub Control", NULL);
      XmtAddDeleteCallback(qmon_arsub, XmDO_NOTHING, qmonARSubPopdown,  NULL);
#endif

      /* 
      ** create the attached modal dialogs for shell, mail, pe, env, ctx
      **
      */
      qmonARSubCreateDialogs(qmon_arsub);

      /*
      ** initialize the ARSMData struct
      */
      qmonInitARSMData(&ARSMData);
   
      /*
      ** set duration as default
      */
      XmToggleButtonSetState(arsub_duration_toggle, True, True);
   }

   /* 
   ** start up AR timer  
   */
   qmonStartTimer(AR_T | PE_T);

   /*
   ** reset; fill the dialog in qalter mode
   */
   qmonARSubClear(w, NULL, NULL);
   
   /*
   ** set arsub_mode_data
   */
   if (data) {
      arsub_mode_data.mode = data->mode;
      arsub_mode_data.ar_id = data->ar_id;
      /* set dialog title */
      sprintf(buf, "Alter AR " sge_u32, data->ar_id);
      xtitle = XmtCreateXmString(buf); 
      XtVaSetValues( qmon_arsub,
                     XmNdialogTitle, xtitle,
                     NULL);
      XmStringFree(xtitle);
   } 
   else {
      arsub_mode_data.mode = ARSUB_NORMAL;
      arsub_mode_data.ar_id = 0;
      xtitle = XmtCreateLocalizedXmString(qmon_arsub, "@{Submit AR}");
      XtVaSetValues( qmon_arsub,
                     XmNdialogTitle, xtitle,
                     NULL);
      XmStringFree(xtitle);
   }

   if (arsub_mode_data.mode == ARSUB_QALTER_PENDING) {
      lListElem *ar_to_set;

      qmonMirrorMultiAnswer(AR_T, &alp);
      if (alp) {
         qmonMessageBox(w, alp, 0);
         lFreeList(&alp);
         /* set default cursor */
         XmtDisplayDefaultCursor(w);
         DEXIT;
         return;
      }
      ar_to_set = ar_list_locate(qmonMirrorList(SGE_AR_LIST), 
                                   arsub_mode_data.ar_id);

      /*
      ** for debugging
      */
      if (rmon_mlgetl(&RMON_DEBUG_ON, GUI_LAYER) & INFOPRINT) {
         printf("___AR_BEFORE_ALTER_____________________\n");
         lWriteElemTo(ar_to_set, stdout);
         printf("________________________________________\n");
      }

      qmonCullToSM(ar_to_set, &ARSMData);
      XmtDialogSetDialogValues(arsub_layout, &ARSMData);

      setButtonLabel(arsub_arsub, "@{Qalter AR}");
      /*
      ** change the resources pixmap icon if necessary
      */
      qmonARSubChangeResourcesPixmap();
   }
   else {
      setButtonLabel(arsub_arsub, "@{Submit AR}");
   }
      

   /*
   ** set sensitivity depending on arsub mode
   ** if called in ARSUB_QALTER_PENDING mode
   */
   qmonARSubSetSensitive(arsub_mode_data.mode);

   /*
   ** pop it up and raise to top of window stack
   */
   xmui_manage(qmon_arsub);

   /* set default cursor */
   XmtDisplayDefaultCursor(w);
   
   DEXIT;
}   


/*-------------------------------------------------------------------------*/
lList *qmonARSubHR(void)
{
   return ARSMData.resource_list;
}
   
/*-------------------------------------------------------------------------*/
String qmonARSubRequestType(void)
{
   static char buf[BUFSIZ];

   DENTER(GUI_LAYER, "qmonARSubRequestType");

   XmtDialogGetDialogValues(arsub_layout, &ARSMData);

   if (ARSMData.pe) {
      sprintf(buf, 
              XmtLocalize(arsub_layout, "@fBParallel AR Request - %s",
                           "@fBParallel AR Request - %s"),
              ARSMData.pe); 
   }
   else
      strcpy(buf, 
             XmtLocalize(arsub_layout, "@fBSerial AR", "@fBSerial AR"));
                           

   DEXIT;
   return buf;
}

/*-------------------------------------------------------------------------*/
/* P R I V A T E                                                           */
/*-------------------------------------------------------------------------*/
static Widget qmonARSubCreate(
Widget parent 
) {
   Widget arsub_handle_hard_error;

   DENTER(GUI_LAYER, "qmonARSubCreate");

   /* create children */
   arsub_layout = XmtBuildQueryDialog( parent, "qmon_arsub",
                          arsm_resources, XtNumber(arsm_resources),
                          "arsub_mail_userPB", &arsub_mail_userPB,
                          "arsub_pePB", &arsub_pePB,
                          "arsub_arsub", &arsub_arsub,
                          "arsub_done", &arsub_done,
                          "arsub_resources", &arsub_resources,
                          "arsub_mail", &arsub_mail,
                          "arsub_mail_user", &arsub_mail_user,
                          "arsub_pe", &arsub_pe,
                          "arsub_start_time", &arsub_start_time,
                          "arsub_start_timePB", &arsub_start_timePB,
                          "arsub_end_time", &arsub_end_time,
                          "arsub_end_timePB", &arsub_end_timePB,
                          "arsub_duration_row", &arsub_duration_row,
                          "arsub_duration", &arsub_duration,
                          "arsub_durationPB", &arsub_durationPB,
                          "arsub_duration_toggle", &arsub_duration_toggle,
                          "arsub_handle_hard_error", &arsub_handle_hard_error,
                          "arsub_ckpt_obj", &arsub_ckpt_obj,
                          "arsub_ckpt_objPB", &arsub_ckpt_objPB,
                          "arsub_clear", &arsub_clear,
                          "arsub_name", &arsub_name,
                          "arsub_main_link", &arsub_main_link,
                          "arsub_message", &arsub_message,
                          NULL);
   
   /*
   ** in SGE mode the project field and the duration time have to be
   ** displayed, otherwise they are unmanaged
   */
   XtAddCallback(arsub_durationPB, XmNactivateCallback, 
                  qmonARSubDuration, (XtPointer)arsub_duration);
   XtManageChild(arsub_layout);

   /* callbacks */
   XtAddCallback(arsub_main_link, XmNactivateCallback, 
                     qmonMainControlRaise, NULL);
   XtAddCallback(arsub_name, XmtNverifyCallback, 
                     qmonARSubCheckARName, NULL);
   XtAddCallback(arsub_arsub, XmNactivateCallback, 
                     qmonARSubARSub, NULL);
   XtAddCallback(arsub_resources, XmNactivateCallback, 
                     qmonPopupARSubResource, NULL);
   XtAddCallback(arsub_done, XmNactivateCallback, 
                     qmonARSubPopdown, NULL);
   XtAddCallback(arsub_clear, XmNactivateCallback, 
                     qmonARSubClear, NULL);
   XtAddCallback(arsub_mail_userPB, XmNactivateCallback, 
                     qmonARSubMailList, NULL);
   XtAddCallback(arsub_ckpt_objPB, XmNactivateCallback, 
                     qmonARSubAskForCkpt, NULL);
   XtAddCallback(arsub_pePB, XmNactivateCallback, 
                     qmonARSubAskForPE, NULL);
   XtAddCallback(arsub_start_timePB, XmNactivateCallback, 
                     qmonARSubStartEndTime, (XtPointer) arsub_start_time);
   XtAddCallback(arsub_end_timePB, XmNactivateCallback, 
                     qmonARSubStartEndTime, (XtPointer) arsub_end_time);
   XtAddCallback(arsub_duration_toggle, XmNvalueChangedCallback, 
                     qmonARSubToggleDuration, NULL);

   XtAddEventHandler(XtParent(arsub_layout), StructureNotifyMask, False, 
                        SetMinShellSize, NULL);
   XtAddEventHandler(XtParent(arsub_layout), StructureNotifyMask, False, 
                        SetMaxShellSize, NULL);

   DEXIT;
   return arsub_layout;

}



/*-------------------------------------------------------------------------*/
static void qmonARSubPopdown(Widget w, XtPointer cld, XtPointer cad)
{

   DENTER(GUI_LAYER, "qmonARSubPopdown");

   if (qmon_arsub) {
      qmonStopTimer(AR_T | PE_T);
      xmui_unmanage(qmon_arsub);
   } 
   DEXIT;
}   

/*-------------------------------------------------------------------------*/
static void qmonARSubSetSensitive(int mode)
{
   Boolean sensitive;

   DENTER(GUI_LAYER, "qmonARSubSetSensitive");

   if (mode == ARSUB_NORMAL)
      sensitive = True;
   else
      sensitive = False;
   
   /*
   ** main arsub dialogue section
   */
   XtSetSensitive(arsub_start_time, sensitive);
   XtSetSensitive(arsub_start_timePB, sensitive);
   XmtChooserSetSensitive(arsub_mail, 0, sensitive);
   XmtChooserSetSensitive(arsub_mail, 1, sensitive);
   XmtChooserSetSensitive(arsub_mail, 3, sensitive);
   XtSetSensitive(arsub_duration, sensitive);
   XtSetSensitive(arsub_durationPB, sensitive);

   DEXIT;
}   


/*-------------------------------------------------------------------------*/
/* set sensitivity of end/duration input                                  */
/*-------------------------------------------------------------------------*/
static void qmonARSubToggleDuration(Widget w, XtPointer cld, XtPointer cad)
{  
   XmToggleButtonCallbackStruct *cbs = (XmToggleButtonCallbackStruct*)cad;

   DENTER(GUI_LAYER, "qmonARSubToggleDuration");
   
   if (cbs->set) {
      XtSetSensitive(arsub_end_time, False);
      XtSetSensitive(arsub_end_timePB, False);
      XtSetSensitive(arsub_duration, True);
      XtSetSensitive(arsub_durationPB, True);
   }
   else {
      XtSetSensitive(arsub_end_time, True);
      XtSetSensitive(arsub_end_timePB, True);
      XtSetSensitive(arsub_duration, False);
      XtSetSensitive(arsub_durationPB, False);
   }
   
   DEXIT;
}   

/*-------------------------------------------------------------------------*/
/* popup XmtAskForString Dialog, get start/end time and set it in input field   */
/* if inputfield is empty or 0 there are no restrictions on exec time      */
/*-------------------------------------------------------------------------*/
static void qmonARSubStartEndTime(Widget w, XtPointer cld, XtPointer cad)
{  
   Widget time_field = (XtPointer) cld;
   Boolean status;
   char message[] = "@{arsub.askarsubtime.Enter the Start/End time in the\nfollowing format: [[CC]]YY]MMDDhhmm[.ss]\nor leave the current time and press ok}";
   char start_time[128];
   lList *alp = NULL;
   dstring ds;
   char buffer[128];
   String current;

   DENTER(GUI_LAYER, "qmonARSubStartEndTime");

   sge_dstring_init(&ds, buffer, sizeof(buffer));

   current = XmtInputFieldGetString(time_field);
   if (current && current[0] != '\0') {
      sge_strlcpy(start_time, current, sizeof(start_time));
   } else {
      sge_strlcpy(start_time, sge_at_time(0, &ds), sizeof(start_time));    
   }

   status = XmtAskForString(w, NULL, message, 
                           start_time, sizeof(start_time), 
                           NULL);
   /* 
   ** validate start_time and show warning msgbox
   */
   if (status) {
      u_long32 tmp_date_time;

      ulong_parse_date_time_from_string(&tmp_date_time, &alp, start_time);
      if (alp) {
         qmonMessageBox(w, alp, 0);
         lFreeList(&alp);
      } else {
         XmtInputFieldSetString(time_field, start_time);
      }   
   }

   DEXIT;
}   

/*-------------------------------------------------------------------------*/
/* popup XmtAskForString Dialog, get duration time                         */
/* if inputfield is empty or 0 there are no restrictions on exec time      */
/*-------------------------------------------------------------------------*/
static void qmonARSubDuration(Widget w, XtPointer cld, XtPointer cad)
{
   Widget input_field = (Widget) cld;
   char stringval[256];
   Boolean status = False;
   String current;
   Boolean mode = True;

   DENTER(GUI_LAYER, "qmonARSubDuration");

   /*
   ** no infinity
   */
   mode = False;

   current = XmtInputFieldGetString(input_field);
   sge_strlcpy(stringval, current, sizeof(stringval));
   status = XmtAskForTime(w, NULL, "@{Enter time}",
               stringval, sizeof(stringval), NULL, mode);
   if (stringval[0] == '\0')
      status = False;

   if (status)
      XmtInputFieldSetString(input_field, stringval);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
/* get the dialog entries and send gdi request                             */
/*-------------------------------------------------------------------------*/
static void qmonARSubARSub(Widget w, XtPointer cld, XtPointer cad)
{
   lList *lp = NULL, *alp = NULL;
   lEnumeration *what = NULL;
   char buf[BUFSIZ];
   Boolean status = False;
   int just_verify = 0;
  
   DENTER(GUI_LAYER, "qmonARSubARSub");

   /* set busy cursor */
   XmtDisplayBusyCursor(w);

   /* 
   **get the values from the dialog fields
   */
   XmtDialogGetDialogValues(arsub_layout, &ARSMData);

   if (arsub_mode_data.mode != ARSUB_QALTER_PENDING) {
      /*
      ** validate input, return error message
      */
      if (ARSMData.pe && ARSMData.pe[0] != '\0') {
         char theInput[1024];
         char *pe = NULL;
         char *pe_range = NULL;
         lList *alp = NULL;
   
         sge_strlcpy(theInput, ARSMData.pe, 1023);
         pe = strtok(theInput, " ");
         pe_range = strtok(NULL, "\n");
         if (!(pe_range && pe_range[0] != '\0')) {
            sprintf(buf, 
               XmtLocalize(w, 
               "Parallel Environment requires valid name and valid range !", 
               "Parallel Environment requires valid name and valid range !")
            );
            goto error;
         } else {
            lList *range_list = NULL;

            range_list_parse_from_string(&range_list, &alp, pe_range,
                                         1, 0, INF_ALLOWED);
            lFreeList(&range_list);
            if (alp) {
               sprintf(buf, 
                  XmtLocalize(w, 
                  "Parallel Environment requires valid name and valid range !", 
                  "Parallel Environment requires valid name and valid range !")
               );
               lFreeList(&alp);
               goto error;
            }
         }
      }

      if (!(lp = lCreateElemList("ARSubList", AR_Type, 1))) {
         DPRINTF(("lCreateElemList failure\n"));
         sprintf(buf, 
                 XmtLocalize(w, 
                             "AR submission failed", 
                             "AR submission failed")
         );
         goto error;
      }

      if (!qmonSMToCull(&ARSMData, lFirst(lp), 0)) {
         DPRINTF(("qmonSMToCull failure\n"));
         sprintf(buf, 
                 XmtLocalize(w, 
                             "AR submission failed", 
                             "AR submission failed")
         );
         goto error;
      }

      if (rmon_mlgetl(&RMON_DEBUG_ON, GUI_LAYER) & INFOPRINT) {
         printf("___SUBMITTED_AR_________________________\n");
         lWriteListTo(lp, stdout);
         printf("________________________________________\n");
      }

      just_verify = (lGetUlong(lFirst(lp), AR_verify) ==
                           JUST_VERIFY);

      what = lWhat("%T(ALL)", AR_Type);
      alp = qmonAddList(SGE_AR_LIST, qmonMirrorListRef(SGE_AR_LIST), 
                        AR_id, &lp, NULL, what);

      if (lFirst(alp) && (lGetUlong(lFirst(alp), AN_status) == STATUS_OK
            || lGetUlong(lFirst(alp), AN_quality) != ANSWER_QUALITY_ERROR))
         status = True;

      qmonMessageBox(w, alp, just_verify);

      if (status && !just_verify) {
         updateARList();
         XmtMsgLinePrintf(arsub_message, 
                           XmtLocalize(w, "AR %d submitted", "AR %d submitted"), 
                           (int)lGetUlong(lFirst(lp), AR_id));
         XmtMsgLineClear(arsub_message, DISPLAY_MESSAGE_DURATION); 
      }
      else if (!just_verify) {
         int ar_id;
         if (lFirst(lp) && (ar_id = (int)lGetUlong(lFirst(lp), AR_id))) {
            XmtMsgLinePrintf(arsub_message, 
                             XmtLocalize(w, "AR %d failed", "AR %d failed"),
                             ar_id); 
         } else {
            XmtMsgLinePrintf(arsub_message, 
                             XmtLocalize(w, "AR Submission failed", 
                                          "AR Submission failed")); 
         }
         XmtMsgLineClear(arsub_message, DISPLAY_MESSAGE_DURATION); 
      }
      lFreeWhat(&what);
      lFreeList(&lp);
      lFreeList(&alp);
   }
   else {
      Boolean close_dialog = True;
      /* should be the same fields like in tARSMEntry in qmon_arsub.h */
      static int fixed_qalter_fields[] = {
         AR_id,
         AR_name,
         AR_start_time,
         AR_end_time,
         AR_resource_list,
         AR_mail_options,
         AR_mail_list,
         AR_account,
         AR_checkpoint_name,
         AR_pe_range,
         AR_pe,
         AR_queue_list,
         AR_verify,
         AR_type,
         AR_acl_list,
         AR_xacl_list,
         NoName
      };
      int qalter_fields[100];
      int i;
      lEnumeration *what;
      lDescr *rdp;

      /* initialize int array */
      qalter_fields[0] = NoName;

      /* add all standard qalter fields */
      for (i=0; fixed_qalter_fields[i]!= NoName; i++)
         nm_set((int*)qalter_fields, (int)fixed_qalter_fields[i]);

      /* 
      ** the duration initiation time 
      ** can be modified if the user is a duration user 
      */
      qmonMirrorMultiAnswer(USERSET_T, &alp);
      if (alp) {
         qmonMessageBox(w, alp, 0);
         /* set default cursor */
         XmtDisplayDefaultCursor(w);
         lFreeList(&alp);
         DEXIT;
         return;
      }

      if (!(what = lIntVector2What(AR_Type, (int*) qalter_fields))) {
         DPRINTF(("lIntVector2What failure\n"));
         sprintf(buf, "AR modify operation failed\n");
         goto error;
      }

      rdp = NULL;
      lReduceDescr(&rdp, AR_Type, what);
      if (!rdp) {
         DPRINTF(("lReduceDescr failure\n"));
         sprintf(buf, "failed to build reduced descriptor\n");
         goto error;
      }
      lFreeWhat(&what);
      
      if (!(lp = lCreateElemList("ARSubList", rdp, 1))) {
         DPRINTF(("lCreateElemList failure\n"));
         sprintf(buf, "AR submission failed\n");
         goto error;
      }

      lSetUlong(lFirst(lp), AR_id, arsub_mode_data.ar_id);
      
      if (!qmonSMToCull(&ARSMData,lFirst(lp), 0)) {
         DPRINTF(("qmonSMToCull failure\n"));
         sprintf(buf, "AR submission failed\n");
         goto error;
      }

      if (rmon_mlgetl(&RMON_DEBUG_ON, GUI_LAYER) & INFOPRINT) {
         printf("___QALTERED_AR________________________\n");
         lWriteListTo(lp, stdout);
         printf("________________________________________\n");
      }

      alp = ctx->gdi(ctx, SGE_AR_LIST, SGE_GDI_MOD, &lp, NULL, NULL);
      if (!qmonMessageBox(w, alp, 0)) {
         updateARListCB(w, NULL, NULL);
         XmtMsgLinePrintf(arsub_message, "AR %d altered", 
                           (int)lGetUlong(lFirst(lp), AR_id));
         XmtMsgLineClear(arsub_message, DISPLAY_MESSAGE_DURATION); 
         close_dialog = True;
      } 
      else {
         XmtMsgLinePrintf(arsub_message, "Alter AR %d failed",
                           (int)lGetUlong(lFirst(lp), AR_id));
         XmtMsgLineClear(arsub_message, DISPLAY_MESSAGE_DURATION); 
         close_dialog = False;
      }

      lFreeList(&lp);
      lFreeList(&alp);
      
      if (close_dialog)
         qmonARSubPopdown(w, NULL, NULL); 
   
   }
   
   /* 
   ** set normal cursor and discard click ahead
   */
   XmtDiscardButtonEvents(w); 
   XmtDisplayDefaultCursor(w);

   DEXIT;
   return;

   error:
      qmonMessageShow(w, True, buf);
      XmtDisplayDefaultCursor(w);
      DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonInitARSMData(
tARSMEntry *data 
) {
   DENTER(GUI_LAYER, "qmonInitARSMData");
   
   memset((void*)data, 0, sizeof(tARSMEntry));
   data->verify_mode = SKIP_VERIFY;

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonFreeARSMData(
tARSMEntry *data 
) {
   DENTER(GUI_LAYER, "qmonFreeARSMData");
   
   if (data->ar_name) {
      XtFree((char*)data->ar_name);
      data->ar_name = NULL;
   }

   if (data->ckpt_obj) {
      XtFree((char*)data->ckpt_obj);
      data->ckpt_obj = NULL;
   }

   if (data->account_string) {
      XtFree((char*)data->account_string);
      data->account_string = NULL;
   }   

   if (data->pe) {
      XtFree((char*)data->pe);
      data->pe = NULL;
   }   

   lFreeList(&(data->mail_list));

   lFreeList(&(data->resource_list));

   lFreeList(&(data->queue_list));

   DEXIT;
}

/*-------------------------------------------------------------------------*/
/*  the ar element to dialog data conversion                              */
/*  we need a valid tARSMEntry pointer                                       */
/*-------------------------------------------------------------------------*/
static Boolean qmonCullToSM(
lListElem *jep,
tARSMEntry *data
) {
   char pe_tasks[BUFSIZ];
   StringConst ar_name;
   StringConst account_string;
   StringConst pe;
   StringConst ckpt_obj;
   const char* username = ctx->get_username(ctx);
   const char* qualified_hostname = ctx->get_qualified_hostname(ctx);
   
   DENTER(GUI_LAYER, "qmonCullToSM");

   /*
   ** free any allocated memory
   */
   qmonFreeARSMData(data); 
   qmonInitARSMData(data);


   /* 
   ** read in default values from a job 
   */
   if ((ar_name = (StringConst)lGetString(jep, AR_name)))
      data->ar_name = XtNewString(ar_name);
   else
      data->ar_name = NULL;
      
   if ((ckpt_obj = (StringConst)lGetString(jep, AR_checkpoint_name)))
      data->ckpt_obj = XtNewString(ckpt_obj);
   else
      data->ckpt_obj = NULL;

   if ((account_string = (StringConst)lGetString(jep, AR_account)))
      data->account_string = XtNewString(account_string);
   else
      data->account_string = NULL;

   data->mail_list = lCopyList("AR_mail_list", lGetList(jep, AR_mail_list));
   if (!data->mail_list) {
      lListElem* entry = lAddElemStr(&(data->mail_list), MR_user, 
                                    username, MR_Type);
      if (entry)
         lSetHost(entry, MR_host, qualified_hostname);
   }

   data->mail_options = MailOptionsToDialog(lGetUlong(jep, AR_mail_options));

   data->start_time = lGetUlong(jep, AR_start_time);
   data->end_time = lGetUlong(jep, AR_end_time);
   data->duration = lGetUlong(jep, AR_duration);

   data->resource_list = lCopyList("AR_resource_list", lGetList(jep, AR_resource_list));
   data->queue_list = lCopyList("AR_queue_list", 
                                    lGetList(jep, AR_queue_list));;

   if ((pe = (StringConst)lGetString(jep, AR_pe))) {
      dstring range_string = DSTRING_INIT;

      range_list_print_to_string(lGetList(jep, AR_pe_range), &range_string, 1);
      sprintf(pe_tasks, "%s %s", pe, sge_dstring_get_string(&range_string));  
      sge_dstring_free(&range_string);
      data->pe = XtNewString(pe_tasks);
   }
   else
      data->pe = NULL;

   DEXIT;
   return True;
}

/*-------------------------------------------------------------------------*/
/* we need a valid ar element pointer                                     */
/*-------------------------------------------------------------------------*/
static Boolean qmonSMToCull(
tARSMEntry *data,
lListElem *jep,
int save 
) {
   char pe_tasks[BUFSIZ];
   char *pe = NULL;
   char *pe_range = NULL;
   lList *perl = NULL;
   lList *alp = NULL;
   int reduced_ar;
   const char *username = ctx->get_username(ctx);
   const char *qualified_hostname = ctx->get_qualified_hostname(ctx);
   
   DENTER(GUI_LAYER, "qmonSMToCull");

   reduced_ar = !(arsub_mode_data.mode != ARSUB_QALTER_PENDING && arsub_mode_data.mode != ARSUB_QALTER_RUNNING ); 

   if (!data->ar_name || data->ar_name[0] == '\0') {
      lSetString(jep, AR_name, NULL);
   } else {
      lSetString(jep, AR_name, data->ar_name);
   }   
   
   lSetString(jep, AR_checkpoint_name, data->ckpt_obj);

   /* 
    * process the resources from dialog
    */ 
   lSetUlong(jep, AR_start_time, data->start_time);

   if (XmToggleButtonGetState(arsub_duration_toggle)) {
      data->end_time = data->start_time + data->duration;
   } else {
      data->duration = data->end_time - data->start_time;
   }
   lSetUlong(jep, AR_end_time, data->end_time);
   lSetUlong(jep, AR_duration, data->duration);

   lSetString(jep, AR_account, data->account_string);
 
   /* default mailer address */
   if (!data->mail_list && !save) {
      data->mail_list = lCreateElemList("ML", MR_Type, 1);
      if (data->mail_list) {
         lSetString(lFirst(data->mail_list), MR_user, username);
         lSetHost(lFirst(data->mail_list), MR_host, qualified_hostname);
      }
   }
   lSetList(jep, AR_mail_list, lCopyList("ML", data->mail_list));
   DPRINTF(("AR_mail_list %p\n", data->mail_list));
   
   lSetUlong(jep, AR_mail_options, ConvertMailOptions(data->mail_options));

   lSetUlong(jep, AR_verify, data->verify_mode);

   DPRINTF(("data->resource_list is %s\n", 
            data->resource_list ? "NOT NULL" : "NULL"));
   lSetList(jep, AR_resource_list, 
               lCopyList("resource_list", data->resource_list));


   DPRINTF(("data->queue_list is %s\n", 
            data->queue_list ? "NOT NULL" : "NULL"));
   lSetList(jep, AR_queue_list, 
               lCopyList("queue_list", data->queue_list));
   
   DPRINTF(("data->pe is %s\n", data->pe ? data->pe: "NULL"));
   if (data->pe && data->pe[0] != '\0') {
      strcpy(pe_tasks, data->pe);
      pe = strtok(pe_tasks, " ");
      pe_range =  strtok(NULL, "\n");
      range_list_parse_from_string(&perl, &alp, pe_range,
                                   0, 0, INF_ALLOWED);
      if (pe && perl && !alp) { 
         lSetString(jep, AR_pe, pe);
         lSetList(jep, AR_pe_range, perl);
      }
      else {
         lFreeList(&alp);
         return False;
      }
   }

   DEXIT;
   return True;
}

/*-------------------------------------------------------------------------*/
static void qmonARSubCheckARName(Widget w, XtPointer cld, XtPointer cad)
{
   XmtInputFieldCallbackStruct *cbs = (XmtInputFieldCallbackStruct*)cad;
   
   DENTER(GUI_LAYER, "qmonARSubCheckARName");
   if (!(cbs->input) || (cbs->input[0] == '\0')) {
      qmonMessageShow(w, True, "@{No AR Name specified !}");
      cbs->okay = False;
      DEXIT;
      return;
   }

   if (strchr(cbs->input, '/')) {
      qmonMessageShow(w, True, "@{AR Name must not contain / !}");
      cbs->okay = False;
   } 

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static u_long32 ConvertMailOptions(
int mail_options 
) {
   u_long32 out = 0;
   int i;
   static u_long32 mail_at[] = { MAIL_AT_BEGINNING, MAIL_AT_EXIT, 
                                 MAIL_AT_ABORT, MAIL_AT_SUSPENSION};
   
   DENTER(GUI_LAYER, "ConvertMailOptions");

   if (mail_options) {
      for (i=0; i<4; i++) {
         if ((mail_options & (1<<i)) == (1<<i))
            out |= mail_at[i]; 
      }
   }
   else
      out = NO_MAIL;
      
   DPRINTF(("mail_options = 0x%08x\n", out));

   DEXIT;
   return out;
}

/*-------------------------------------------------------------------------*/
static int MailOptionsToDialog(
u_long32 mail_options 
) {
   int out = 0;
   int i;
   static u_long32 mail_at[] = { MAIL_AT_BEGINNING, MAIL_AT_EXIT, 
                                 MAIL_AT_ABORT, MAIL_AT_SUSPENSION};
   
   DENTER(GUI_LAYER, "ConvertMailOptions");

   if (mail_options) {
      for (i=0; i<4; i++) {
         if ((mail_options & mail_at[i]))
            out += (1<<i);
      }
   }
      
   DPRINTF(("mail_options = 0x%08x, out = %d\n", mail_options, out));

   DEXIT;
   return out;
}

/*-------------------------------------------------------------------------*/
/* sub dialogs mail list                                                   */
/*-------------------------------------------------------------------------*/
static void qmonARSubCreateDialogs(
Widget w 
) {
   Widget armatrix, arreset, arcancel, arokay, ardelete,
          armail_list_new;

   DENTER(GUI_LAYER, "qmonARSubCreateDialogs");

   /* create subdialogs */

   if (!armail_list_w) {
      armail_list_w = XmtBuildQueryDialog( w, 
                                       "arsub_mail_list_shell", 
                                       arsm_resources, XtNumber(arsm_resources),
                                       "armail_list_matrix", &armatrix, 
                                       "armail_list_cancel", &arcancel,
                                       "armail_list_okay", &arokay,
                                       "armail_list_reset", &arreset,
                                       "armail_list_delete", &ardelete,
                                       "armail_list_new", &armail_list_new,
                                       NULL
                                       );
      XtRealizeWidget(armail_list_w);

      XtAddCallback(arreset, XmNactivateCallback, 
                     qmonARSubReset, (XtPointer) armail_list_w);
      XtAddCallback(arokay, XmNactivateCallback,
                     qmonARSubOkay, (XtPointer) armail_list_w);
      XtAddCallback(arcancel, XmNactivateCallback,
                     qmonARSubCancel, (XtPointer) armail_list_w);
      XtAddCallback(ardelete, XmNactivateCallback,
                     qmonARSubDelete, (XtPointer) armatrix);
      XtAddCallback(armail_list_new, XmtNinputCallback,
                     qmonARSubMailInput, (XtPointer) armatrix);

   }

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonARSubAskForPE(Widget w, XtPointer cld, XtPointer cad)
{
   Boolean status = False;
   lList *pel = NULL;
   lListElem *pep = NULL;
   int n, i;
   StringConst *strs = NULL;
   static char buf[BUFSIZ];
   
   DENTER(GUI_LAYER, "qmonARSubAskForPE");
   
   pel = qmonMirrorList(SGE_PE_LIST);
   n = lGetNumberOfElem(pel);
   if (n>0) {
      strs = (StringConst*)XtMalloc(sizeof(String)*n); 
      for (pep=lFirst(pel), i=0; i<n; pep=lNext(pep), i++) {
        /*
        ** we get only references don't free, the strings
        */
        strs[i] = (StringConst)lGetString(pep, PE_name);
      }
    
      strcpy(buf, "");
      /* FIX_CONST_GUI */
      status = XmtAskForItem(w, NULL, "@{Select a Parallel Environment}",
                        "@{Available Parallel Environments}", 
                        (String*) strs, n, False, buf, BUFSIZ, NULL); 
      
      if (status) {
         strcat(buf, " 1");
         XmtInputFieldSetString(arsub_pe, buf);
         XmProcessTraversal(arsub_pe, XmTRAVERSE_CURRENT);
/*          XmTextSetHighlight(arsub_pe,  */
/*                             XmTextGetLastPosition(arsub_pe) - 1,  */
/*                             XmTextGetLastPosition(arsub_pe), */
/*                             XmHIGHLIGHT_SELECTED); */
         XmTextSetInsertionPosition(arsub_pe,
                                    XmTextGetLastPosition(arsub_pe));
      }
      /*
      ** don't free referenced strings, they are in the pel list
      */
      XtFree((char*)strs);
   }
   else
      qmonMessageShow(w, True, 
            "@{Please configure a Parallel Environment first !}");
   
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonARSubAskForCkpt(Widget w, XtPointer cld, XtPointer cad)
{
   Boolean status = False;
   lList *ckptl = NULL;
   lListElem *cep = NULL;
   int n, i;
   StringConst *strs = NULL;
   static char buf[BUFSIZ];
   lList *alp = NULL;
   
   DENTER(GUI_LAYER, "qmonARSubAskForCkpt");
   
   qmonMirrorMultiAnswer(CKPT_T, &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      lFreeList(&alp);
      DEXIT;
      return;
   }
   ckptl = qmonMirrorList(SGE_CKPT_LIST);
   n = lGetNumberOfElem(ckptl);
   if (n>0) {
      strs = (StringConst*)XtMalloc(sizeof(String)*n); 
      for (cep=lFirst(ckptl), i=0; i<n; cep=lNext(cep), i++) {
        /*
        ** we get only references don't free, the strings
        */
        strs[i] = (StringConst)lGetString(cep, CK_name);
      }
    
      strcpy(buf, "");
      /* FIX_CONST_GUI */
      status = XmtAskForItem(w, NULL, "@{Select a checkpoint object}",
                        "@{Available checkpoint objects}", 
                        (String*) strs, n, False, buf, BUFSIZ, NULL); 
      
      if (status) {
         XmtInputFieldSetString(arsub_ckpt_obj, buf);
      }
      /*
      ** don't free referenced strings, they are in the pel list
      */
      XtFree((char*)strs);
   }
   else
      qmonMessageShow(w, True, 
            "@{Please configure a checkpoint object first !}");
   
   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonARSubMailList(Widget w, XtPointer cld, XtPointer cad)
{
   DENTER(GUI_LAYER, "qmonARSubMailList");
   /* 
   ** get the values from the dialog fields, if there have been entries
   ** in the main dialog get them to set them in the subdialog
   */
   XmtDialogGetDialogValues(arsub_layout, &ARSMData);
   
   /*
   ** set the entries in the helper dialog and pop it up
   */
   XmtDialogSetDialogValues(armail_list_w, &ARSMData);
   XtManageChild(armail_list_w);
   
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonARSubMailInput(Widget w, XtPointer cld, XtPointer cad)
{
   Widget list = (Widget) cld;
   String address;
   XmString item;
   
   DENTER(GUI_LAYER, "qmonARSubMailInput");

   address = XmtInputFieldGetString(w);
   if (address && address[0] != '\0') {
      item = XmStringCreateLtoR(address, XmFONTLIST_DEFAULT_TAG);
      XmListAddItem(list, item, 0);
      XmStringFree(item);
   }
    
   XmtInputFieldSetString(w, "");
   
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonARSubClear(Widget w, XtPointer cld, XtPointer cad)
{
   DENTER(GUI_LAYER, "qmonARSubClear");
   
   /*
   ** free currently set lists and reset to default values
   */
   qmonFreeARSMData(&ARSMData);
   qmonInitARSMData(&ARSMData);

   XmtDialogSetDialogValues(arsub_layout, &ARSMData);

   /*
   ** change the resources pixmap icon if necessary
   */
   qmonARSubChangeResourcesPixmap();

   /*
   ** clear message line
   */
   XmtMsgLineClear(arsub_message, XmtMsgLineNow); 

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonARSubReset(Widget w, XtPointer cld, XtPointer cad)
{
   Widget dialog = (Widget)cld;
   
   DENTER(GUI_LAYER, "qmonARSubReset");

   XmtDialogSetDialogValues(dialog, &ARSMData);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonARSubDelete(Widget w, XtPointer cld, XtPointer cad)
{
   Widget list = (Widget)cld;
   XmString *selectedItems;
   Cardinal selectedItemCount;
   Cardinal itemCount;
   
   DENTER(GUI_LAYER, "qmonARSubDelete");

   XtVaGetValues( list,
                  XmNselectedItemCount, &selectedItemCount,
                  XmNselectedItems, &selectedItems,
                  XmNitemCount, &itemCount,
                  NULL);

   if (selectedItemCount == 0) {
      DEXIT;
      return;
   }

   if (selectedItemCount == itemCount) {
      XmtDisplayWarning(w, "onemailaddress", "There must be at least one mail address.");
      DEXIT;
      return;
   }

   XmListDeleteItems(list, selectedItems, selectedItemCount);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonARSubOkay(Widget w, XtPointer cld, XtPointer cad)
{
   Widget dialog = (Widget)cld;
   
   DENTER(GUI_LAYER, "qmonARSubOkay");

   /*
   ** write the values from the subdialog into ARSMData
   */
   XmtDialogGetDialogValues(dialog, &ARSMData);
    
#if FIXME
   if ((ARSMData.pe && !ARSMData.pe_range) || (!ARSMData.pe && ARSMData.pe_range)) {
      qmonMessageShow(w, True, "Range required for Parallel Environment !");
      DEXIT;
      return;
   }
#endif

   /*
   ** set the new ARSMData into the main arsub dialog
   */
   XmtDialogSetDialogValues(arsub_layout, &ARSMData);

   xmui_unmanage(dialog);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonARSubCancel(Widget w, XtPointer cld, XtPointer cad)
{
   Widget dialog = (Widget)cld;
   
   DENTER(GUI_LAYER, "qmonARSubCancel");

   xmui_unmanage(dialog);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
/* change the resources pixmap                                             */
/*-------------------------------------------------------------------------*/
static void qmonARSubChangeResourcesPixmap(void) 
{
   static Pixmap pix1 = None, pix2 = None;
   static int have_resources = 0;

   DENTER(GUI_LAYER, "qmonARSubChangeResourcesPixmap");

   if (pix1 == None || pix2 == None) {
      pix1 = qmonGetIcon("resources");
      pix2 = qmonGetIcon("resources_enabled");
      if (pix1 == None || pix2 == None)
         DPRINTF(("Pixmap can't be loaded in qmonARSubChangeResourcesPixmap\n"));
   }
  
   if (ARSMData.resource_list) {
      if (!have_resources) {
         have_resources = 1;
         XtVaSetValues(arsub_resources, XmNlabelPixmap, pix2, NULL);
      }
   }
   else {
      if (have_resources) {
         have_resources = 0;
         XtVaSetValues(arsub_resources, XmNlabelPixmap, pix1, NULL);
      }
   }

   DEXIT;
}

/*-------------------------------------------------------------------------*/
void qmonPopupARSubResource(Widget w, XtPointer cld, XtPointer cad)
{
   DENTER(TOP_LAYER, "qmonPopupARSubResource");

   if (!arresource) {
      qmonCreateARSubResource(w, NULL);
   }   
   xmui_manage(arresource);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonCreateARSubResource(Widget parent, XtPointer cld)
{
   Widget   arresource_ok, arresource_cancel, arresource_clear;
 
   DENTER(GUI_LAYER, "qmonCreateARSubResource");

   arresource = XmtBuildQueryDialog(parent, "arresource_shell",
                           NULL, 0,
                           "arresource_ok", &arresource_ok,
                           "arresource_cancel", &arresource_cancel,
                           "arresource_ar", &arresource_ar,
                           "arresource_sr", &arresource_sr,
                           "arresource_clear", &arresource_clear,
                           NULL);
   /*
   ** preset the resources 
   */
   qmonARResourceSetResources(arresource, NULL, NULL);

   XtAddCallback(arresource_ok, XmNactivateCallback,
                        qmonARResourceOK, NULL);
   XtAddCallback(arresource_cancel, XmNactivateCallback,
                        qmonARResourceCancel, NULL);

   /*
   ** arresource callbacks
   */
   XtAddCallback(arresource_ar, XmNactivateCallback,
                        qmonARResourceEditResource, (XtPointer)0);
   XtAddCallback(arresource_sr, XmNactivateCallback,
                        qmonARResourceEditResource, (XtPointer)1);
   XtAddCallback(arresource_sr, XmNremoveCallback, 
                     qmonARResourceRemoveResource, NULL);
   XtAddCallback(arresource_clear, XmNactivateCallback,
                        qmonARResourceClearResources, NULL);
   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonARResourceClearResources(Widget w, XtPointer cld, XtPointer cad)
{
   DENTER(GUI_LAYER, "qmonARResourceClearResources");

   lFreeList(&arresource_resources);
   qmonRequestDraw(arresource_sr, arresource_resources, 1);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonARResourceSetResources(Widget w, XtPointer cld, XtPointer cad)
{
   lList *arl = NULL;
   lListElem *ep = NULL;
   lListElem *rp = NULL;
   lList *alp = NULL;

   DENTER(GUI_LAYER, "qmonARResourceSetResources");

   qmonMirrorMultiAnswer(CENTRY_T, &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      lFreeList(&alp);
      DEXIT;
      return;
   }
   
   arl = qmonGetResources(qmonMirrorList(SGE_CENTRY_LIST), ALL_RESOURCES);

   for_each (ep, arresource_resources) {
      rp = lGetElemStr(arl, CE_name, lGetString(ep, CE_name));
      if (!rp)
         rp = lGetElemStr(arl, CE_shortcut, lGetString(ep, CE_name));
      if (rp) {
         lSetString(ep, CE_name, lGetString(rp, CE_name));
         lSetUlong(ep, CE_valtype, lGetUlong(rp, CE_valtype));
      }
   }
      
   /*
   ** do the drawing
   */
   qmonRequestDraw(arresource_ar, arl, 0);
   qmonRequestDraw(arresource_sr, arresource_resources, 1);

   lFreeList(&arl);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonARResourceEditResource(Widget w, XtPointer cld, XtPointer cad)
{
   XmIconListCallbackStruct *cbs = (XmIconListCallbackStruct*) cad;
   long how = (long)cld;
   lList *arl = NULL;
   int type;
   char stringval[CL_MAXHOSTLEN];
   Boolean status = False;
   StringConst name, value, strval;
   Boolean found = False;
   lListElem *fill_in_request = NULL;
   lListElem *global_fill_in_request = NULL;
   

   DENTER(GUI_LAYER, "qmonARResourceEditResource");

   arl = qmonGetResources(qmonMirrorList(SGE_CENTRY_LIST), ALL_RESOURCES);


   if (!how) {
      global_fill_in_request = lGetElemStr(arl, CE_name, cbs->element->string[0]);
   }
   for_each (fill_in_request, arresource_resources) {
      name = lGetString(fill_in_request, CE_name);
      value = lGetString(fill_in_request, CE_stringval);
      if (cbs->element->string[0] && name && 
         !strcmp(cbs->element->string[0], name)) {
            found = True;
            break;
      }
   }       
         
   if (!found) {
      fill_in_request = global_fill_in_request; 
   }

   if (!fill_in_request) {
      DEXIT;
      return;
   }

   type = lGetUlong(fill_in_request, CE_valtype);
   strval = lGetString(fill_in_request, CE_stringval);
   sge_strlcpy(stringval, strval, CL_MAXHOSTLEN);

   status = qmonRequestInput(w, type, cbs->element->string[0], 
                              stringval, sizeof(stringval));
   /* 
   ** put the value in the CE_Type elem 
   */
   if (status) {
      lListElem *ep = NULL;
      lSetString(fill_in_request, CE_stringval, stringval);
    
      /* put it in the hard or soft resource list if necessary */
      if (!arresource_resources) {
         arresource_resources = lCreateList("arresource_resources", CE_Type);
      }
      if (!(ep = lGetElemStr(arresource_resources, CE_name, cbs->element->string[0]))) {
         lAppendElem(arresource_resources, lCopyElem(fill_in_request));
      } else {
         lSetString(ep, CE_stringval, stringval);
      }   
         
      qmonRequestDraw(arresource_sr, arresource_resources, 1);
   }
   lFreeList(&arl);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonARResourceRemoveResource(Widget w, XtPointer cld, XtPointer cad)
{
   XmIconListCallbackStruct *cbs = (XmIconListCallbackStruct*) cad;
   lListElem *dep = NULL;
   Boolean found = False;
   StringConst name, value;

   DENTER(GUI_LAYER, "qmonARSubRemoveResource");

   if (arresource_resources) {
      for_each (dep, arresource_resources) {
         name = lGetString(dep, CE_name);
         value = lGetString(dep, CE_stringval);
         if (cbs->element->string[0] && name && 
            !strcmp(cbs->element->string[0], name) &&
            cbs->element->string[2] && value &&
            !strcmp(cbs->element->string[2], value) ) {
               found = True;
               break;
         }
      }       
            
      if (found) {
         lRemoveElem(arresource_resources, &dep);
         if (lGetNumberOfElem(arresource_resources) == 0)
            lFreeList(&arresource_resources);
         qmonRequestDraw(arresource_sr, arresource_resources, 1);
      }
   }
   
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonARResourceOK(Widget w, XtPointer cld, XtPointer cad)
{
   DENTER(GUI_LAYER, "qmonARResourceOK");

   lFreeList(&(ARSMData.resource_list));
   ARSMData.resource_list = arresource_resources;
   arresource_resources = NULL;
   /*
   ** change the resources pixmap icon if necessary
   */
   qmonARSubChangeResourcesPixmap();


   xmui_unmanage(arresource);

   DEXIT;
}
   
/*-------------------------------------------------------------------------*/
static void qmonARResourceCancel(Widget w, XtPointer cld, XtPointer cad)
{
   DENTER(GUI_LAYER, "qmonARResourceCancel");
   
   lFreeList(&arresource_resources);
   xmui_unmanage(arresource);

   DEXIT;
}

