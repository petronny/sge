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
/**
 *  Generated from java_jni_event_client.jsp
 *  !!! DO NOT EDIT THIS FILE !!!
 */
<%
  com.sun.grid.cull.JavaHelper jh = (com.sun.grid.cull.JavaHelper)params.get("javaHelper");
  com.sun.grid.cull.CullDefinition cullDef = (com.sun.grid.cull.CullDefinition)params.get("cullDef");
%>
package com.sun.grid.jgdi.jni;


import com.sun.grid.jgdi.JGDIException;
import com.sun.grid.jgdi.JGDI;

/**
 *  EventClient implemention which uses native parts
 *
 *  </pre>
 *  @author  richard.hierlmeier@sun.com 
 *
 */
public class EventClientImpl extends AbstractEventClient implements com.sun.grid.jgdi.EventClient {

   /**
    *   Create a new event client
    *
    *   @param jgdi   gdi connection
    *   @param regId  event client registration id. If this id is 0 the event
    *                 client id will be dynamically assigned
    *   @throws JGDIException If the native intialization of the event client
    *                        fails.
    *                  
    */
   public EventClientImpl(JGDI jgdi, int regId) throws JGDIException {
      super(jgdi, regId);
   }

<%
    java.util.Iterator iter = cullDef.getObjectNames().iterator();
    com.sun.grid.cull.CullObject cullObj = null;
    String name = null;

    while( iter.hasNext() ) {
      name = (String)iter.next();
      cullObj = cullDef.getCullObject(name); 
      
      name = cullObj.getIdlName();
      
      if(name == null) {
         throw new IllegalStateException("Have no idl name for " + cullObj.getName());
      }
      
      if(cullObj.hasAddEvent()) {
%>
     /**
      *  Subscribe/Unsubscribe the add <%=name%> event
      *
      *  @param subscribe Subscribe/Unsubscribe flag
      *  @throws JGDIException if the subcribtion is failed
      */
     public void subscribeAdd<%=name%>(boolean subscribe) throws JGDIException {
        synchronized(syncObject) {
           nativeSubscribeAdd<%=name%>(subscribe);
        }
     }
     
     private native void nativeSubscribeAdd<%=name%>(boolean subscribe) throws JGDIException;
     
     /**
      *  Set the flush time for the add <%=name%> event.
      *
      *  @param  flush     flush flag. (If <code>true</code> flushing is enabled and vice versa)
      *  @param  interval  flush interval in seconds
      *  @throws JGDIException on any error
      */
     public void setAdd<%=name%>Flush(boolean flush, int interval) throws JGDIException {
        synchronized(syncObject) {
           nativeSetAdd<%=name%>Flush(flush, interval);
        }
     }

     private native void nativeSetAdd<%=name%>Flush(boolean flush, int interval) throws JGDIException;
     
     /**
      * Get the flush time of the add <%=name%> event
      *
      * @return the flush time of the add <%=name%> event in seconds
      */
     public int getAdd<%=name%>Flush() throws JGDIException {
        synchronized(syncObject) {
           return nativeGetAdd<%=name%>Flush();
        }
     }
     
     private native int nativeGetAdd<%=name%>Flush() throws JGDIException;
<%
      }
      
      if(cullObj.hasDeleteEvent()) { 
%>
     /**
      *  Subscribe/Unsubscribe the del <%=name%> event
      *
      *  @param subscribe Subscribe/Unsubscribe flag
      *  @throws JGDIException if the subcribtion is failed
      */
     public void subscribeDel<%=name%>(boolean subscribe) throws JGDIException {
        synchronized(syncObject) {
           nativeSubscribeDel<%=name%>(subscribe);
        }
     }

     private native void nativeSubscribeDel<%=name%>(boolean subscribe) throws JGDIException;
     
     /**
      *  Set the flush time for the del <%=name%> event.
      *
      *  @param  flush     flush flag. (If <code>true</code> flushing is enabled and vice versa)
      *  @param  interval  flush interval in seconds
      *  @throws JGDIException on any error
      */
     public void setDel<%=name%>Flush(boolean flush, int interval) throws JGDIException {
        synchronized(syncObject) {
           nativeSetDel<%=name%>Flush(flush, interval);
        }
     }
     
     private native void nativeSetDel<%=name%>Flush(boolean flush, int interval) throws JGDIException;
     
     /**
      * Get the flush time of the del <%=name%> event
      *
      * @return the flush time of the add <%=name%> event in seconds
      */
     public int  getDel<%=name%>Flush() throws JGDIException {
        synchronized(syncObject) {
           return nativeGetDel<%=name%>Flush();
        }
     }
     
     private native int nativeGetDel<%=name%>Flush() throws JGDIException;
     
<%     
      }
      if (cullObj.hasGetListEvent()) {
%>         
     /**
      *  Subscribe/Unsubscribe the list <%=name%> event
      *
      *  @param subscribe Subscribe/Unsubscribe flag
      *  @throws JGDIException if the subcribtion is failed
      */
     public void subscribeList<%=name%>(boolean subscribe) throws JGDIException {
        synchronized(syncObject) {
           nativeSubscribeList<%=name%>(subscribe);
        }
     }
     
     private native void nativeSubscribeList<%=name%>(boolean subscribe) throws JGDIException;
     
     /**
      *  Set the flush time for the list <%=name%> event.
      *
      *  @param  flush     flush flag. (If <code>true</code> flushing is enabled and vice versa)
      *  @param  interval  flush interval in seconds
      *  @throws JGDIException on any error
      */
     public void setList<%=name%>Flush(boolean flush, int interval) throws JGDIException {
        synchronized(syncObject) {
           nativeSetList<%=name%>Flush(flush, interval);
        }
     }
     
     private native void nativeSetList<%=name%>Flush(boolean flush, int interval) throws JGDIException;
     
     /**
      * Get the flush time of the list <%=name%> event
      *
      * @return the flush time of the add <%=name%> event in seconds
      */
     public int getList<%=name%>Flush() throws JGDIException {
        synchronized(syncObject) {
           return nativeGetList<%=name%>Flush();
        }
     }
     
     private native int nativeGetList<%=name%>Flush() throws JGDIException;
     
<%         
      } // end if hasGetListOperation
      if (cullObj.hasModifyEvent()) {
%>      
     /**
      *  Subscribe/Unsubscribe the modify <%=name%> event
      *
      *  @param subscribe Subscribe/Unsubscribe flag
      *  @throws JGDIException if the subcribtion is failed
      */
     public void subscribeMod<%=name%>(boolean subscribe) throws JGDIException {
        synchronized(syncObject) {
           nativeSubscribeMod<%=name%>(subscribe);
        }
     }
     
     private native void nativeSubscribeMod<%=name%>(boolean subscribe) throws JGDIException;
     
     /**
      *  Set the flush time for the del <%=name%> event.
      *
      *  @param  flush     flush flag. (If <code>true</code> flushing is enabled and vice versa)
      *  @param  interval  flush interval in seconds
      *  @throws JGDIException on any error
      */
     public void setMod<%=name%>Flush(boolean flush, int interval) throws JGDIException {
        synchronized(syncObject) {
           nativeSetMod<%=name%>Flush(flush, interval);
        }
     }

     private native void nativeSetMod<%=name%>Flush(boolean flush, int interval) throws JGDIException;
     
     /**
      * Get the flush time of the mod <%=name%> event
      *
      * @return the flush time of the add <%=name%> event in seconds
      */
     public int  getMod<%=name%>Flush() throws JGDIException {
        synchronized(syncObject) {
           return nativeGetMod<%=name%>Flush();  
        }
     }
     
     private native int  nativeGetMod<%=name%>Flush() throws JGDIException;
     
<% 
      } // end of hasModifyOperation
   } // end of while 
   String [] specialEvents = {
       "QmasterGoesDown",
       "SchedulerRun",
       "Shutdown",
       "JobFinish",
       "JobUsage",
       "JobFinalUsage", 
       "JobPriorityMod",
       "QueueInstanceSuspend",
       "QueueInstanceUnsuspend"
   };
   
   for(int i = 0; i < specialEvents.length; i++) {

%>
     /**
      *  Subscribe/Unsubscribe the <%=specialEvents[i]%> event
      *
      *  @param subscribe Subscribe/Unsubscribe flag
      *  @throws JGDIException if the subcribtion is failed
      */
     public void subscribe<%=specialEvents[i]%>(boolean subscribe) throws JGDIException {
        synchronized(syncObject) {
           nativeSubscribe<%=specialEvents[i]%>(subscribe);  
        }
     }
   
     private native void nativeSubscribe<%=specialEvents[i]%>(boolean subscribe) throws JGDIException;
   
     /**
      *  Set the flush time for the <%=specialEvents[i]%> event.
      *
      *  @param  flush     flush flag. (If <code>true</code> flushing is enabled and vice versa)
      *  @param  interval  flush interval in seconds
      *  @throws JGDIException on any error
      */
     public void set<%=specialEvents[i]%>Flush(boolean flush, int interval) throws JGDIException {
        synchronized(syncObject) {
           nativeSet<%=specialEvents[i]%>Flush(flush, interval);  
        }
     }
     
     private native void nativeSet<%=specialEvents[i]%>Flush(boolean flush, int interval) throws JGDIException;
     
     /**
      * Get the flush time of the <%=specialEvents[i]%>  event
      *
      * @return the flush time of the <%=specialEvents[i]%> event in seconds
      */
     public int  getMod<%=specialEvents[i]%>Flush() throws JGDIException {
        synchronized(syncObject) {
           return nativeGet<%=specialEvents[i]%>Flush();  
        }
     }
     
     private native int  nativeGet<%=specialEvents[i]%>Flush() throws JGDIException;
     
     
<%
   } // end of for special events
%>

}
