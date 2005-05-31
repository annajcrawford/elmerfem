#*******************************************************************************
#*
#*       ELMER, A Computational Fluid Dynamics Program.
#*
#*       Copyright 1st April 1995 - , Center for Scientific Computing,
#*                                    Finland.
#*
#*       All rights reserved. No part of this program may be used,
#*       reproduced or transmitted in any form or by any means
#*       without the written permission of CSC.
#*
#*******************************************************************************

#*******************************************************************************
#*
#* Initialize some global TCL variables, and include other TCL/TK commands
#*
#*******************************************************************************
#*
#*                     Author:       Juha Ruokolainen
#*
#*                    Address: Center for Scientific Computing
#*                                Tietotie 6, P.O. BOX 405
#*                                  02101 Espoo, Finland
#*                                  Tel. +358 0 457 2723
#*                                Telefax: +358 0 457 2302
#*                              EMail: Juha.Ruokolainen@csc.fi
#*
#*                       Date: 26 Sep 1995
#*
#*                Modified by:
#*
#*       Date of modification:
#*
#*******************************************************************************
set tcl_interactive 1

set platform $tcl_platform(platform)

if { ![info exists elmerpost_id] } {
  set elmerpost_id elmerpost-[pid]
}

if { $platform == "windows" } {
  catch { package require dde 1.0 }
  catch { dde servername $elmerpost_id }
} else {
  tk appname $elmerpost_id
}


if { [info exists env(ELMER_POST_HOME)] } {
   set ELMER_POST_HOME [file join "" $env(ELMER_POST_HOME)]
} else {
   set ELMER_POST_HOME "/mnt/mds/csc/jpr/SRC/ELMER/PostProcessor/"
}

if { [info exists env(LD_LIBRARY_PATH)] } {

   set LD_LIBRARY_PATH $env(LD_LIBRARY_PATH)

}

set elm_auto_load_packages 1
set global_limits(MAX_COLORMAP_ENTRIES) 512

set auto_noexec 1
#set auto_path "$auto_path ."

#history keep 200

wm geometry . -30+30

proc $    { args } { eval exec $args }
proc @    { args } {
 set a [eval expr $args];
 return $a
}
proc echo { args } { global main_out; $main_out insert end $args\n; $main_out yview end }
proc cat  { args } { $ cat $args }

proc m { args }  { math $args }

proc display {} { translate -r 0 0 0; ActivateGraphicsWindow; }
proc p    {} { display }
proc play {} { display }

proc do { var beg end body {doupdate 0} } {
   global errorInfo errorCode BreakLoop
   upvar $var v

   set do_focus [focus]

   for { set v $beg } { $v <= $end } { incr v } {
       switch [catch {uplevel $body} msg] {
          1 {return -code error -errorinfo $errorInfo -errorcode $errorCode $msg}
          2 {return -code return $msg}
          3 return
       }

       if { $BreakLoop } break

       if { $doupdate != 0 } {
           update
           UpdateDisplay
       }
   }

   focus $do_focus
}

proc forever { body }  {
    global BreakLoop

    while { !$BreakLoop } { 
       switch [catch {uplevel $body} msg] {
          1 {return -code error -errorinfo $errorInfo -errorcode $errorCode $msg}
          2 {return -code return $msg}
          3 return
       }

       if { $BreakLoop } break

       update
       UpdateDisplay
   }
}

rename scale slider
proc Exit {} { exit }

proc clear { {onoff "on"} } {
   global GraphicsClearOn

   if { $onoff == "on" } {
       set GraphicsClearOn 1
   } else {
       set GraphicsClearOn 0
   }
}

proc place_window { win } {
  set main [wm geometry .]
  scan $main "%dx%d%1s%d%1s%d" w h s1 x s2 y
  wm geometry $win $s1[@ $x+100]$s2[@ $y+200]
}

source $ELMER_POST_HOME/tcl/menu.tcl
source $ELMER_POST_HOME/tcl/readfile.tcl
source $ELMER_POST_HOME/tcl/math.tcl
source $ELMER_POST_HOME/tcl/args.tcl
source $ELMER_POST_HOME/tcl/misc.tcl
source $ELMER_POST_HOME/tcl/file.tcl
source $ELMER_POST_HOME/tcl/text.tcl
source $ELMER_POST_HOME/tcl/dialog.tcl
source $ELMER_POST_HOME/tcl/colormap.tcl
source $ELMER_POST_HOME/tcl/group.tcl
source $ELMER_POST_HOME/tcl/material.tcl
source $ELMER_POST_HOME/tcl/timestep.tcl
source $ELMER_POST_HOME/tcl/camera.tcl
source $ELMER_POST_HOME/tcl/clip.tcl

source $ELMER_POST_HOME/tcl/mesh.tcl
source $ELMER_POST_HOME/tcl/vectors.tcl
source $ELMER_POST_HOME/tcl/contours.tcl
source $ELMER_POST_HOME/tcl/isosurface.tcl
source $ELMER_POST_HOME/tcl/particle.tcl
source $ELMER_POST_HOME/tcl/sphere.tcl
source $ELMER_POST_HOME/tcl/colorscale.tcl
source $ELMER_POST_HOME/tcl/saveimage.tcl
source $ELMER_POST_HOME/tcl/LoadSicopolis.tcl

#source $ELMER_POST_HOME/tcl/message.tcl

source $ELMER_POST_HOME/help/help.tcl

material -diffuse 90,90,90 -specular 0,0,0 -opacity 100 -shininess 20
background 0 0 0
colormap $ELMER_POST_HOME/lib/colormaps/default.cm
rotpriority local
UpdateObject

math "source(\"$ELMER_POST_HOME/lib/mc.ini\")"

#
# load shared modules from post/modules directory
#
if { [file exists $ELMER_POST_HOME/modules] } {
  set mods [glob $ELMER_POST_HOME/modules/*];
  
  set n [llength $mods];
  do i 0 [@ $n-1] {
     set mod [lindex $mods $i];
     if { [file exists $mod] } {
        puts "Loading external module: $mod";
        load $mod [file rootname [file tail $mod]]
     }
  }
}

puts "done initializing"


