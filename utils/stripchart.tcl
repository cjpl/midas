#!/bin/sh
#-*-tcl-*-
# the next line restarts using wish  \
exec bltwish "$0" -- ${1+"$@"}

#=========================================================================
# Gertjan Hofman, U. of Colorado, Boulder, March-00.
# 
# tcl program to replace the gstripchart thingy that comes with
# gnome
#
#
# requires:  tcl + tk + BLT = bltwish
#
# v 1.0  Mar - 00 for PAA/Triumf/Midas
#
# There are a few tricky things going on here.
# Since passing TCL lists to the graph seems to have a memory leak,
# I use BLT vectors, which are hot linked to the graph elements.
# Since I dont know a priori how many vectors to create, I the name
# of the item that I am plotting as the vector name, preceded by
# V_x_ or V_y_.
# To access an individual data point, which I need for the scrolling
# now have to do a double de-refencing. Say we create a vector
# vector create my_vect(10)
# then I can access the vector using   puts $my_vect(3)  
# but if I have
# set name my_vect
# vector create $x
# Now I need to translate x--> my_vect --> its value.
# you can do this in two ways:
# puts [set [set x](0)]  or  puts [ set ${x}(0)] or even
# puts [set $x\(0)].  In the first case, the inner 'set' returns
# my_vect. This is then appended to (0) and the second set returns
# the result. In the last case, the \ 'escapes' or seperates the (0)
# as not being part of the name. etc etc. Think about it.
#
# v. 1.0  -added help menu, added smarter scaling of of the expanded
#          graphs.
# v  1.1  -added mouse select of graph items
# v. 2.0  -added support for mhist (odb history)
# v. 2.1  -added postscript support + zooming of single graph windows
# 
# things to do:
# - improve help/info menu's
# - trap errors ! 
# - check compatibility with gchart, esp. reading of fields
# - what about simple arithmetic ? (a la gchart).
# - think of some cool back grounds (chaos.../midas etc)
#
#  Revision History:
#    $Log$
#    Revision 1.4  2000/05/01 16:34:41  pierre
#    - Added File .hst selection
#    - Improve X-axis labels
#
#    Revision 1.3  2000/04/20 17:06:50  pierre
#    # ODB or history stripchart in tcl. Requires bltwhish. (New application)
#
#
#=========================================================================


namespace import blt::*     ;# get BLT commands imported

#============================================================
# SHOW MESSAGE   - pop up message window and display error
#============================================================

proc show_message {message} {
    global message_done                      ;# indicate whether message has been read
    toplevel .messages                       ; # create a new window
    wm title .messages "stripchart warning"
    label    .messages.1   -text  "$message"
    pack     .messages.1   -side  top
    button   .messages.ok  -text  "OK" -command {
	destroy .messages 
	set message_done 1                  
	return}
    pack     .messages.ok  -side  bottom 
    update
}


#=============================================================
# Convert clock seconds to HH:MM:SS format
#=============================================================
# this function converts a 'x' axis value to a time format according to
# a format statement passed to it.
proc my_clock_format {format  graph ltime} {
   return "[clock format  [expr round( ($ltime/60.)*60)] -format $format]"
}


#=============================================================
# EXIT procedure
#=============================================================
proc nice_exit {} {
    exit
return
}


#==============================================================
# READ THE MCHART CONFIGURATION FILE
#==============================================================  
proc read_conf_file {conf_fname} {

    global item_fname item_fields item_pattern item_equation
    global item_color item_max item_min item_list
    global equip_name

    # open the file, get ptr. 
    set file_hndl [open $conf_fname r]
    
    while   {! [eof $file_hndl]} {
	
	gets $file_hndl string
	
	# skip over comments ...except catch Pierre's equipment name.
	if {[string first "#Equipment:" $string] != -1 } {
	    set equip_name [lindex $string 1]
	}
	
	if {[string index $string 0]=="#" || [llength $string]==0} {
	    continue
	}
	set string [string tolower $string]           ;# convert to lower case
	
	#disect string to obtain info. Look for begin line.
	# note: string first return location of first occurance -1 if not found
	if { [string first "begin:" $string] != -1} {
	    
	    # now fetch the second item - the value name
	    
	    set item [lindex $string 1]             ;# assume its the second item
	    
	    set item [filter_bad_chars $item]
	    
	    lappend item_list $item                 ;# create a list of items for looping
	    
	    # ok we got a begin. Check for the usefil items
	    while { [string first "end:" $string] == -1 } {
		gets $file_hndl string
		switch -glob -- [lindex $string 0] {      
		    "filename:" { set item_fname($item)    [lindex $string 1] }
		    "fields:"   { set item_fields($item)   [lindex $string 1] }
		    "pattern:"  { set item_pattern($item)  [lindex $string 1] }
		    "equation:" { set item_equation($item) [lindex $string 1] }
		    "color:"    { set item_color($item)    [lindex $string 1] }
		    "maximum:"  { set item_max($item)      [lindex $string 1] }
		    "minimum:"  { set item_min($item)      [lindex $string 1] }
		    "begin:"    {
			show_message  "Error in .conf file -  begin but no previous end "
			after 5000
			exit  
		    }
		}
	    } 
	}
    }
    
    close $file_hndl


# do some checks on the max/min values, in case Pierre screwed them up
    foreach item $item_list {
	if { $item_max($item)==0 && $item_min($item)==0} {
	    puts "found zero max/min values for $item. Correcting"
	    set item_max($item) 1
	}
    }
    
    
    return
}


#======================================================================
# GRAPH INDIVIDUAL ITEM ON FULL SCALE
#======================================================================

proc select_graph {item} {
# here we show the REAL data, unscaled, on a seperate graph
# note: we want to be able to crack open multiple graphs
# so the names of the widget need to be a variable.
# can't remember how to make static variables, so make it
# global

#    global V_x_$item                ;# vectors associated with the plotte item
#    global V_y_$item
    global item_color               ;# colour associated with item
    global scale_ymin scale_ymax    ;# present best y-scale
    global winsize_x winsize_y      ;# size of toplevel and graph windows
    global doing_mhist

# check if window exist already - dont replot the same item twice
    if [winfo exists .fullscale$item ] {
	wm deiconify .fullscale$item          ;# de-iconize it it
	raise        .fullscale$item          ;# raise to foreground
	return
    }

    toplevel .fullscale$item   ;# create new window with unique name
    wm title .fullscale$item "$item"

# split screen in two parts -
    frame .fullscale$item.col1 ;# create to columes - rhs for graph
    frame .fullscale$item.col2 ;# left hand side for buttons.
    pack .fullscale$item.col1 -side left
    pack .fullscale$item.col2 -side right -fill both -expand 1


    set fgraph .fullscale$item.col2.graph

    graph $fgraph -title "" -relief ridge -bd 3     ;# make a new graph
    $fgraph configure -width 5.0i -height 2.0i      ;# configure it a little 
    $fgraph legend configure -position @31,8  -anchor nw  -relief raised 

    pack $fgraph -side right -fill both -expand 1

    # now, autoscaling may not be the best. Lets try use the standard
    # deviation for the data points that we have stored right now.
    calc_best_scale $item   ;# returns calc values, or "" if not enough data

    $fgraph yaxis configure -max $scale_ymax -min $scale_ymin

    # add day of the week if using the history command
    if {$doing_mhist} {
	$fgraph xaxis configure -loose 0 -title "" -command {my_clock_format "%d.%m %H:%M"}
    } else {
	$fgraph xaxis configure -loose 0 -title "" -command {my_clock_format %H:%M}
    }
	
    $fgraph element create line 
    $fgraph element configure line  \
	    -label $item -color $item_color($item) -symbol "" \
	    -xdata V_x_$item -ydata V_y_$item

    # puts "data are now [set V_y_[set item](:)]"
    # create  exit button  
    button   .fullscale$item.col1.ok  -text  "close"  -width 6 -font 6x12 \
	    -command "destroy .fullscale$item  ;  return"
    pack     .fullscale$item.col1.ok  -side bottom

    # create re-scaling button
    
    button   .fullscale$item.col1.scale1  -text  "AutoScale"  -width 6 -font 6x12 \
	    -command "scale_single_window Auto $item" 
    pack     .fullscale$item.col1.scale1  -side top
    button   .fullscale$item.col1.scale2  -text  "ReScale" -font 6x12 -width 6 \
	    -command "scale_single_window Rescale $item"
    pack     .fullscale$item.col1.scale2  -side top   

    # create information button
    button   .fullscale$item.col1.info  -text  "Info/Help"  -width 6 -font 6x12 \
	    -command "show_item_info $item" 
    pack     .fullscale$item.col1.info  -side top

    # create hard copy pull down menu:
    menubutton .fullscale$item.col1.hard -text "HardCopy" \
	    -menu .fullscale$item.col1.hard.mnu -relief raised -font 6x12 

    set hardcopy_menu [ menu  .fullscale$item.col1.hard.mnu]

    $hardcopy_menu add command -command "hardcopy ps  $fgraph" -label "ps  file"
    $hardcopy_menu add command -command "hardcopy jpg $fgraph" -label "jpg file"
    $hardcopy_menu add command -command "hardcopy gif $fgraph" -label "gif file"
    $hardcopy_menu add command -command "hardcopy png $fgraph" -label "png file"

    pack  .fullscale$item.col1.hard
    # bindings for the zooming function.
    bind $fgraph <ButtonPress-1> "zoom_select %W %x %y start"
    bind $fgraph <ButtonRelease-1> "zoom_select %W %x %y stop"


    return
}

#======================================================================
# ZOOM ON SELECTED GRAPHS USING MOUSE
#======================================================================
proc zoom_select {window x  y point} {
    global zoom_coor


    if {$point=="start"} {
	set zoom_coor(corner1)  [$window invtransform $x $y]
	return
    } elseif {$point=="stop"} {
	set zoom_coor(corner2)  [$window invtransform $x $y]
    } else {
	tk_messageBox -message "Error: wrong parameter to zoom routine"
	return
    }

    # ok have corner 1 and 2
    # reset graph axis:
    # robustness check:
    if { [lindex $zoom_coor(corner1) 0] <  [lindex $zoom_coor(corner2) 0] } {
	$window xaxis configure -min [lindex $zoom_coor(corner1) 0] -max  [lindex $zoom_coor(corner2) 0]
    }

    return
}

#================================================================
# HARDCOPY TO POSTSCRIPT/JPG/PNG/GIF FILE
#================================================================

proc hardcopy {type window} {
    $window  postscript configure  -paperwidth 6.5i -paperheight 8i \
	    -landscape true 
    $window  postscript output stripchart.ps 

    if {$type =="ps"} {
	tk_messageBox -message "Postscript output to file stripchart.ps"
	return
    }

    if {$type=="jpg"} {
	catch "exec convert stripchart.ps stripchart.jpg" err
	if {$err==""} {
	    tk_messageBox -message "JPEG  output to file stripchart.jpg"
	    return
	} else {
	    tk_messageBox -message "Error: $err.\n  Output defaults to ps file stripchart.ps"
	    return
	}
    } 
   if {$type=="png"} {
	catch "exec convert stripchart.ps stripchart.png" err
	if {$err==""} {
	    tk_messageBox -message "PNG output to file stripchart.png"
	    return
	} else {
	    tk_messageBox -message "Error: $err.\n  Output defaults to ps file stripchart.ps"
	    return
	}
    } 

   if {$type=="gif"} {
	catch "exec convert stripchart.ps stripchart.gif" err
	if {$err==""} {
	    tk_messageBox -message "GIF output to file stripchart.gif"
	    return
	} else {
	    tk_messageBox -message "Error: $err.\n  Output defaults to ps file stripchart.ps"
	    return
	}
    } 

    return
}


#================================================================
# RESET SCALE ON EXANDED WINDOW
#================================================================

proc scale_single_window {scale_mode item} {
    global scale_ymax scale_ymin   ;# returned and calculated values

    set fgraph .fullscale$item.col2.graph
    switch -glob -- $scale_mode {
	"Auto"    { set scale_ymax "" ; set scale_ymin "" } 
	"Rescale" { calc_best_scale $item}    ;# this returns new best values
    }

    $fgraph yaxis configure -min $scale_ymin -max $scale_ymax
    $fgraph xaxis configure -min "" -max ""
    update
    return
}	    


#==================================================================
# CALCULATE Y-SCALE USING DATA VALUE STANDARD DEVIATIONS
#==================================================================

proc calc_best_scale { item } {
# now, autoscaling may not be the best. Lets try use the standard
# deviation for the data points that we have stored right now.
#    global V_x_$item
#    global V_y_$item
    global scale_ymin scale_ymax

    if { [vector expr length(V_y_$item)] > 10 } {
	set stand_dev [ vector expr sdev(V_y_$item) ]  ;# get standard deviation
	set mean      [ vector expr mean(V_y_$item) ]  ;# get the mean
	
	set scale_ymin  [expr $mean - 8.* $stand_dev]  
	set scale_ymax  [expr $mean + 8.* $stand_dev]  
	
	#robustness check:
	if { $scale_ymin >= $scale_ymax } {
#	    tk_messageBox -message "warning: data is constant - no y-scaling \
#		    possible for $item. \n\n  ymax/min = $scale_ymin $scale_ymax \n\
#		    Standard deviation = $stand_dev"
#	    puts "data are now [set V_y_[set item](:)]"
	    set scale_ymin ""
	    set scale_ymax ""
	}
	
    } else {
	set scale_ymin ""
	set scale_ymax ""
    }

    return
}

#====================================================================
# SLEEP ROUTINE (ms)
#====================================================================

proc wait_ms {mill_sec} {
    set mill_sec [expr $mill_sec/100]
    for {set count 0 } { $count < $mill_sec} { incr count} {
	after 100
	update
    }
    return
}

#======================================================================
# SET THE SCROLL TIME INTERVAL USING RADIO BUTTONS
#======================================================================

proc new_scroll_time { } {
    global time_limit

# did the button already get pressed ?
    if [winfo exists .main.radio ] {
	wm deiconify . 
	raise        . 
	return
    }

    frame .main.radio
    raise .
    update
    set radio .main.radio

    radiobutton $radio.radio1 -text  "100 sec " -relief flat \
	    -variable time_limit -value 100.        
    radiobutton $radio.radio2 -text  "5  min  " -relief flat \
	    -variable time_limit -value [expr 5*60.]    
    radiobutton $radio.radio3 -text  "1  hrs  " -relief flat \
	    -variable time_limit -value [expr 60*60.]  
    radiobutton $radio.radio4 -text  "5  hrs  " -relief flat \
	    -variable time_limit -value [expr 5*60*60.]
    radiobutton $radio.radio5 -text  "12  hrs  " -relief flat \
	    -variable time_limit -value [expr 12*60*60.]
    button $radio.ok  -text "ok" -command "destroy $radio"

    pack $radio
    for {set i 1} {$i<6} {incr i} {
	pack $radio.radio$i
    }

    pack $radio.ok

    update
    return
}

#=====================================================================
# SHOW INFO ON INDIVUDUAL ITEM
#=====================================================================

proc show_item_info {item} {
    global equip_name

    toplevel .item_info
    #  put a an <ok> button on it:
    button .item_info.ok -text "OK" -command {destroy .item_info ; return}
    pack   .item_info.ok -side bottom
    
    #  put the text in the window:
    message .item_info.mess -width 6i -text \
	    "note: <spaces> are substituted by _ as are \\+- and % signs \n\n \
	    item name :  $equip_name/ $item \n\n \
	    Available functions: \n\n \
	    Click-and-drag  Left button to zoom time axis.\
	    Click Rescale to return to normal mode\n \
	    Autoscale = optimun scale for graph size \n \
	    ReScale   = scale calculate using data standard deviation\n \
	    HardCopy  = output to JPEG/PS/PNG etc\n \
	    Close     = close this window\n"
	    


    pack .item_info.mess

    return
}


#=======================================================================
# LOCATE GRAPH ITEM POINTED TO BY MOUSE
#=======================================================================

proc mouse_find_item {window x y } {
    global item_list

    # transform X-win coordinates to graph coordinates.
    set graph_coor [$window invtransform $x $y]
    set x_coor [lindex $graph_coor 0]
    set y_coor [lindex $graph_coor 1]
    
    # loop over entire list of data find the closest point and display
    # the item belonging to that point.
    
    set closest [expr 10000000000000000.0]

    foreach item $item_list {
	global V_x_$item
	global V_y_plot_$item
	vector create dist                 ;# dummy vector
	dist expr "(V_x_$item- $x_coor)^2 + (V_y_plot_$item - $y_coor)^2 "

	# rember smallest element
	if { [expr $dist(min)] <  $closest} {
	    set closest [expr $dist(min)]
	    set closest_item $item
	}
    }
    # clean up
    vector destroy dist

    # call the graph spawn routine.
    select_graph $closest_item
    return
}


#========================================================================
# READ THE OUTPUT OF MHIST, ALLOW SELECTION OF DATA AND GRAPH ** MHIST **
#========================================================================
proc read_mhist { } {
    global event_choice
    global pick_event_var
    global event_var
    global history_time history_interval history_unit history_amount


    global item_fname item_fields item_pattern item_equation
    global item_color item_max item_min item_list
    global equip_name
    
    global strip_chart

    global exit_now
    global select_men
    global open_file
    
    global event_ids_info ;#mhist event ID and name
    global event_var      ;#varialbe name

    global debug
    global button_toggle  ; #use to toggle lots of variables on/off simul.
    set error_var ""
    set exit_now  0

    if { [llength $item_list] >0} {
	clean_list_and_vectors_and_widgets          ;# remove all things associate
                                                    ;# with previous data
    }

# note: string compare is like 'c' - 0 == string match !!!
# so if NOT debugging, execute mhist:
    if { [string compare $debug "mhist"] } {
	catch "exec rm -f /tmp/mhist" error_var
	catch "exec /usr/local/bin/mhist -l > /tmp/mhist" error_var
    }

    if {$error_var !="" } {
	tk_messageBox -message "Could not start mhist (path problem ?) \n $error_var"
	return
    }

    # assume the mhist out put is there. Retrieve the ID list
    if {![get_mhist_list]} return
 
    # do selection of the event_id:
    if {[select_event_id]=="do_exit"} return

    # There is also the option to open a .hst file directly to look 
    # at the event ID's (today's event ID's may not be the right ones.
    # for this purpose, Stephan added the -f option to mhist. So if
    # want to open a specific file
    if {$open_file} {
	# here we go..a *new* widget
	set types {{"History files .hst" {.hst}}}
	set hist_file [tk_getOpenFile -filetypes $types]
	# bug ! Mhist doesnt work with full path names. So for now: 
	set hist_file [lindex [split $hist_file /] end]

	if ![string compare $hist_file ""] return
	# ok, so we have the file name. Now repeat the previous excercicse
	# of getting the event ID.
	catch "exec rm -f /tmp/mhist" error_var
	set exec_string "/usr/local/bin/mhist -f $hist_file -l"
	catch "exec $exec_string > /tmp/mhist" error_var
	if {$error_var !="" } {
	    tk_messageBox -message "Could not start mhist (path problem ?) \n $error_var"
	    return
	}
	# ok, repeat ID event selection from new list
	# assume the mhist out put is there
	if {![get_mhist_list]} return
	if {[select_event_id]=="do_exit"} return
	set open_file 1     ;# have to reset open file - last selection reset it to 0
    }

    # ok, now display and pick list of events_names. Use checkbutton
    toplevel .select_vars
    wm title .select_vars "mhist variable select"
    wm geometry .select_vars +[winfo rootx .]+[winfo rooty .]

    frame .select_vars.col1row1  -relief ridge  -bd 3 
    frame .select_vars.col2row1  -relief ridge  -bd 3 
    frame .select_vars.col1row2  -relief ridge  -bd 3 
    frame .select_vars.col2row2  -relief ridge  -bd 3 
    frame .select_vars.col1row3  -relief ridge  -bd 3 
    frame .select_vars.col2row3  -relief ridge  -bd 3 

    grid .select_vars.col1row1 -row 1 -column 1
    grid .select_vars.col2row1 -row 1 -column 2
    grid .select_vars.col1row2 -row 2 -column 1
    grid .select_vars.col2row2 -row 2 -column 2
    grid .select_vars.col1row3 -row 3 -column 1
    grid .select_vars.col2row3 -row 3 -column 2


# ok now try to put half the variables in one column, half in the other
    set i 0
    foreach var $event_var($event_choice) {
	set win_name [filter_bad_chars $var]
	incr i
	set pick_event_var($var) 1
	if {$i <= [expr [ llength $event_var($event_choice)]/2] } {
	    checkbutton .select_vars.col1row2.$win_name  -text $var \
		    -variable pick_event_var($var) -state active   -relief flat -anchor w
	    pack .select_vars.col1row2.$win_name -fill x -expand 1
	} else {
	    checkbutton .select_vars.col2row2.$win_name  -text $var \
		    -variable pick_event_var($var) -state active   -relief flat -anchor w
	    pack .select_vars.col2row2.$win_name -fill x -expand 1
	}
    }

    set button_toggle 1
    checkbutton .select_vars.col1row3.all -text "select all/none" -relief ridge \
	    -variable button_toggle -command { 
	foreach var $event_var($event_choice) {
	    set pick_event_var($var) $button_toggle
	}   
    }

    pack  .select_vars.col1row3.all -fill x -expand 1

    button .select_vars.col2row3.ok  -text "ok" -command "destroy .select_vars "
    button .select_vars.col2row3.cancel \
	    -text "cancel" -command "set exit_now 1 ; destroy .select_vars ; return"

    pack .select_vars.col2row3.ok     -side left
    pack .select_vars.col2row3.cancel -side right


    # add radio buttons for the time duration and interval

    # if using old file, add some text and an 'until now' choice
    if {$open_file} {
	frame .select_vars.col1row0
	grid  .select_vars.col1row0 -row 0 -column 1 -columnspan 2
	label .select_vars.col1row0.text -text "Old history file selected. Displayed \
		interval is \n\ from date of file to date of file plus chosen time below" \
		-relief ridge  -bd 3 
	pack  .select_vars.col1row0.text
    }

    frame .select_vars.col1row1.left           ;# subdivide even further: day/weeks etc
    frame .select_vars.col1row1.right          ;# multiplier
    grid  .select_vars.col1row1.right -column 2 -row 1
    grid  .select_vars.col1row1.left  -column 1 -row 1

    set radio .select_vars.col1row1.left
    set history_time 1

    label .select_vars.col1row1.lab1 -text "History Time:" -anchor center -justify center
    grid  .select_vars.col1row1.lab1 -row 0 -columnspan 2

    label .select_vars.col1row1.left.lab -text "Units"
    pack  .select_vars.col1row1.left.lab
    
    set radio .select_vars.col1row1.left

    # set defualt variable values
    set history_time 1
    set history_unit days

    
    radiobutton $radio.day  -text "days"   -relief flat -anchor w  \
            -variable history_unit   -value days
    pack $radio.day -fill x -expand 1
    radiobutton $radio.week  -text "weeks"   -relief flat -anchor w  \
            -variable history_unit   -value weeks
    pack $radio.week -fill x -expand 1
    radiobutton $radio.month  -text "months"   -relief flat -anchor w  \
            -variable history_unit   -value months
    pack $radio.month -fill x -expand 1

    if {$open_file} {
	radiobutton $radio.until_now  -text "until now"   -relief flat -anchor w  \
		-variable history_unit   -value until_now
	pack $radio.until_now -fill x -expand 1
    }
    
    # now the day/week/month multiplier
    set history_amount 1
    label .select_vars.col1row1.right.lab -text "Multiplier"
    pack  .select_vars.col1row1.right.lab

    set radio .select_vars.col1row1.right
    

    radiobutton $radio.1  -text "1"   -relief flat -anchor w  \
            -variable history_amount   -value 1
    pack $radio.1 -fill x -expand 1
    radiobutton $radio.2  -text "2"   -relief flat -anchor w  \
	    -variable history_amount   -value 2
    pack $radio.2 -fill x -expand 1
    radiobutton $radio.3  -text "3"   -relief flat -anchor w  \
            -variable history_amount   -value 3
    pack $radio.3 -fill x -expand 1
    radiobutton $radio.4  -text "4"   -relief flat -anchor w  \
            -variable history_amount   -value 4
    pack $radio.4 -fill x -expand 1
    radiobutton $radio.5  -text "5"   -relief flat -anchor w  \
	    -variable history_amount   -value 5
    pack $radio.5 -fill x -expand 1
    

    # for the interval:
    set history_interval 300                  ;# default values

    label .select_vars.col2row1.lab -text "History Interval"
    pack  .select_vars.col2row1.lab

    set radio .select_vars.col2row1
    
    radiobutton $radio.1day  -text "100 s"   -relief flat -anchor w  \
            -variable history_interval   -value 100
    pack $radio.1day -fill x -expand 1
    radiobutton $radio.2day  -text "5 mins"   -relief flat -anchor w  \
            -variable history_interval   -value 300
    pack $radio.2day -fill x -expand 1
    radiobutton $radio.3day  -text "30 mins"   -relief flat -anchor w  \
            -variable history_interval   -value [expr 30*60]
    pack $radio.3day -fill x -expand 1
    radiobutton $radio.4day  -text "1 hours"   -relief flat -anchor w  \
            -variable history_interval   -value [expr 60*60]
    pack $radio.4day -fill x -expand 1
    radiobutton $radio.5day  -text "5 hours"   -relief flat -anchor w  \
            -variable history_interval   -value [expr 5*60*60]
    pack $radio.5day -fill x -expand 1
   
     update

    # now wait until the window with the variables/times is gone before going on:

    tkwait window .select_vars
    if {$exit_now} return

    # calculate history time (assume todays history
    switch -glob -- $history_unit {
	"days"   {set history_time [expr $history_amount *1  ] }
	"weeks"  {set history_time [expr $history_amount *7  ] }
	"months" {set history_time [expr $history_amount *30 ] } 
    }

    # deal with the open file possibility
    # when opening old file, assume start date (-s) = name of file
    #                               end   date (-p) = calculated from user input above

    if {$open_file} {
	# calc start date from file name: (hist_file)
	# unfortunately, Stephan uses  yymmdd.hst  ? or ? We need ddmmyy.
	# split list at the '.'
	set date_part [lindex [split $hist_file .] 0]
	set day_part  [string range $date_part 4 5]
	set mon_part  [string range $date_part 2 3]
	set yre_part  [string range $date_part 0 1]
	set start_time $day_part$mon_part$yre_part

	# ok, do the end part.
	if {$history_unit =="until_now"} {
	    set stop_time [clock format [clock seconds] -format "%d%m%y"]
	} else {
	    # gets tough. Must calculate a new day from old date and interval
	    set stop_time [calc_history_stop_time $day_part $mon_part $yre_part]
	} 
	set exec_string \
		"/usr/local/bin/mhist -b -s $start_time \
		-p $stop_time -t $history_interval -e $event_choice"

	# do normal case - present day file
    } else {            

	set exec_string \
		"/usr/local/bin/mhist -b -d $history_time -t $history_interval -e $event_choice"
    }


    # now, now filter out which events we dont want and repeatedly call mhist
    # to get what we want.
    #  -b = time in seconds. 


    foreach  var $event_var($event_choice) {

	# was this guy enabled ?
	if { !$pick_event_var($var)} { continue}

	append exec_string " -v \"$var\" "              ;# surround by quotes
	if { [string compare $debug "mhist"]  } {
	    catch {exec rm -f /tmp/mhist_data} error_var
	    catch "exec $exec_string > /tmp/mhist_data"  error_var
	}

	if {$error_var !=""} {
	    tk_messageBox -message "problem: error from mhist \n  $exec_string \n $error_var"
	    return
	}


	# get ready to create the vector names
	set item $var
	set item [filter_bad_chars $item]           ;# remove spaces, slashes, etc etc

	# create the new vectors
	
	vector create V_x_$item
	vector create V_y_$item
	vector create V_y_plot_$item

	# get the data and read into BLT vectors as usual.
	set file_hndl [open "/tmp/mhist_data" r ]
	
	while {![eof $file_hndl]} {
	    gets $file_hndl string
	    if { [llength $string] == 2} {
		set V_x_${item}(++end) [lindex $string 0]
		set V_y_${item}(++end) [lindex $string 1]
	    } elseif {[llength $string] >=2 } { 
		tk_messageBox -message "problem: looking for data, but found \n \n $string "
		return
	    }
	}

	close $file_hndl

	# fill in the same info we might have gotten from a .conf file:
	# add to the list of items plotted
	lappend item_list $item
	# set the color
	set item_color($item)  [get_new_color]

# now for these data calculate appropiate max/min values, as we were
# given them from the .conf file:
	if { [vector expr length(V_y_$item)] <= 1} {
	    tk_messageBox  -default ok -message "Not enough data read for plotting" -type ok 
	    return
	}

	if { [vector expr length(V_y_$item)] > 10 } {
	    set stand_dev [ vector expr sdev(V_y_$item) ]  ;# get standard deviation
	    set mean      [ vector expr mean(V_y_$item) ]  ;# get the mean
	    
	    set item_min($item)  [expr $mean - 8.* $stand_dev]  
	    set item_max($item)  [expr $mean + 8.* $stand_dev]  
	    
	    #robustness check:
	    if { $item_min($item) >= $item_max($item) } {
		set item_min($item) [expr $mean - 1.]
		set item_max($item) [expr $mean + 1.]
	    }
	} else {
	    set mean      [ vector expr mean(V_y_$item) ] 
	    set item_min($item) [expr $mean - 100.]
	    set item_max($item) [expr $mean + 100.]
	}

	# from this max/min value, calculate the plotted normalized histo values:
	# do a vector calculation on this new guy.

	V_y_plot_$item \
		expr "(V_y_$item -  $item_min($item)) / ($item_max($item)-$item_min($item))"  

	# finally associate/create a graph item for this guy
	if {! [$strip_chart element exists line_$item]} {
	    $strip_chart element create line_$item
	}
	$strip_chart element configure line_$item -label "" -color $item_color($item) -symbol ""
	# now hot-link it to the new graph line 
	$strip_chart element configure line_$item  -xdata V_x_$item  -ydata V_y_plot_$item
	# change the labelling format on the x-axis if plotting more then one day
	if {$history_time >= 1} {
	    $strip_chart xaxis configure -command {my_clock_format "%d.%m %H:%M"}
	} else {
	    $strip_chart xaxis configure -command {my_clock_format "%H:%M"}
	}

	# create the pull down item menu. Also align the text string
	set blank_string "                            "
	set item_length [string length $item]
	if {$item_length > 13} {set item_length 13}
	set menu_string "$item [string range $blank_string 1 [expr 13 - $item_length] ]"
	append menu_string $item_color($item)
	$select_men add command -command "select_graph $item" -label  $menu_string
	

	# next item:
	update

    }


    return
}

#===================================================================
#   CALCULATE HISTORTY STOP TIME FROM START DATE AND INTERVAL
#===================================================================

proc calc_history_stop_time {day_part mon_part yre_part} {
    global history_unit history_amount
    
    switch -glob -- $history_unit {
	"days"   {
	    set lday_part  [expr $day_part + $history_amount ]
	    set lmon_part  $mon_part
	    set lyre_part  $yre_part
	    if {$lday_part > 28} {
		set lmon_part [incr lmon_part]
		set lday_part [expr $lday_part - 28]
	    }
	   
	}
	"weeks"  {
	    set lday_part  [expr $day_part + $history_amount*7 ]
	    set lmon_part  $mon_part
	    set lyre_part  $yre_part
	    if {$lday_part > 28} {
		set lmon_part [incr lmon_part]
		set lday_part [expr $lday_part - 28]
	    }
	}
	"months" {
	    set lday_part  $day_part
	    set lmon_part   [expr $mon_part + $history_amount  ]
	    set lyre_part  $yre_part
	    if {$lmon_part > 12} {
		set lyre_part [incr lyre_part]
		set lmon_part [expr $lmon_part - 12]
	    }    
	}
    }

    # make sure they are all 2 digits
    if {[string length $lday_part] == 1} {
	set lday_part "0$lday_part"
    }
    if {[string length $lmon_part] == 1} {
	set lmon_part "0$lmon_part"
    }
    if {[string length $lyre_part] == 1} {
	set lyre_part "0$lyre_part"
    }

    # ok now put the string back together
    return $lday_part$lmon_part$lyre_part
}

#=========================================================================
#  SELECT THE EVENT ID TO DISPLAY IN MHIST
#=========================================================================


proc select_event_id { } {
    global event_choice
    global pick_event_var
    global event_var
    global history_time history_interval history_unit history_amount


    global item_fname item_fields item_pattern item_equation
    global item_color item_max item_min item_list
    global equip_name
    
    global strip_chart

    global exit_now
    global select_men
    global open_file
    
    global event_ids_info ;#mhist event ID and name
    global event_var      ;#varialbe name

    global debug
    global button_toggle  ; #use to toggle lots of variables on/off simul.

    set open_file 0
    set exit_now  0


    # ok, first throw up a selection window for the event id. I guess
    # this should be a radio button ?
    
    if [winfo exists .select_event ] {
	wm deiconify .select_event 
	raise        .select_event 
    } else {
	toplevel  .select_event
	wm title  .select_event "mhist event select"
	# the next puts the new window at the same position as the root window
	wm geometry .select_event +[winfo rootx .]+[winfo rooty .]
	frame     .select_event.radio
	frame     .select_event.row2
    }
    
    label .select_event.label -text "Please Choose Event Number \n (or \
	    hit `open file' to select an old  history file):"
    pack .select_event.label -side top
    
    set radio .select_event.radio 
    # set default choice
    set event_choice [lindex [lindex $event_ids_info 0] 0]
    
    # note: the variables in a -variable statement MUST be global
    foreach event_id $event_ids_info {
	set win_name [filter_bad_chars [lindex $event_id 0]]
	radiobutton $radio.radio$win_name -text $event_id  -relief flat -anchor w  \
		-variable event_choice   -value [lindex $event_id 0]
	pack $radio.radio$win_name -fill x -expand 1
    }
    
    button .select_event.row2.ok      -text "ok"     -command "destroy .select_event "
    button .select_event.row2.cancel  -text "cancel" -command "set exit_now 1 ; destroy .select_event"
    button .select_event.row2.openfile -text "open file" -command "set open_file 1;destroy .select_event"
    
    pack $radio
    pack .select_event.row2 -side bottom
    pack .select_event.row2.ok -side left
    pack .select_event.row2.cancel -side right
    pack .select_event.row2.openfile -side right
    update
    
    # !! The next command forces the script to halt until the .select_event
    # dialog box has been destroyed. This is the only way to get a 'top down'
    # order of events.
    
    tkwait window .select_event
    
    
    if {$exit_now}  {
	return "do_exit"
    }
    if {$open_file} {
	return "open_file"
    }

    return "continue"
    
}





#===================================================================
# EMPTY THE ITEM LIST DESTROY EXISTING VECTORS
#===================================================================

proc clean_list_and_vectors_and_widgets { } {
    global item_fname item_fields item_pattern item_equation
    global item_color item_max item_min item_list
    global select_men

    $select_men delete 1 [llength $item_list]   ;# remove pull down menu's

    foreach item $item_list {
	vector destroy V_x_$item
	vector destroy V_y_$item
	vector destroy V_y_plot_$item
    }

    set item_list ""
    
    if {[array exist item_fname]}    {unset item_fname}
    if {[array exist item_fields]}   {unset item_fields}
    if {[array exist item_pattern]}  {unset item_pattern}
    if {[array exist item_equation]} {unset item_equation}
    if {[array exist item_color]}    {unset item_color}
    if {[array exist item_max]}      {unset item_max}
    if {[array exist item_min]}      {unset item_min}



    return

}
    
    
#===================================================================
# READ THE MHIST -l OUTPUT TO FIND EVID's, VARIABLE NAMES
#===================================================================

proc get_mhist_list { } {
    global event_ids_info ;#mhist event ID and name
    global event_var      ;#varialbe name

# erase the previous list

    set event_ids_info ""
    if {[array exist event_var]} {unset event_var}

    set file_hndl [open "/tmp/mhist" r]
    
    while   {! [eof $file_hndl]} {
	gets $file_hndl string
# look for a couple of things: ID: give event iD number
# x: gives some of the possible variables.
	if {[string first "Event ID" $string ] !=-1 } {
	    # ok, found an event ID: read the id, name
	    # look for ID and the ':'
	    set pos1  [string first "ID" $string]
	    set pos2 [string first  ":"  $string]
	    set evid [string range $string [expr $pos1 +2 ] [expr $pos2 -1] ]
	    set evid [string trim $evid]
	    set evnam [string range $string [expr $pos2 +1] [string length $string]]
	    set evnam [string trim $evnam]
	    lappend event_ids_info "$evid $evnam"

	    # skip next spaces:
	    set string ""
	    while   {![eof $file_hndl] && [llength $string ] == 0  } { 
		gets $file_hndl string
	    }
	    # read the variables
	    while   {![eof $file_hndl] && [llength $string] != 0 } {
		# split list at :, then grab the second part of the list
		set nam [lindex [split $string :] 1]
		set nam [string trim $nam]
		if {[string length $nam] !=0} {
		    lappend event_var($evid) $nam 
		}
		gets $file_hndl string
	    }
	}
    }

    close $file_hndl

    return 1
}

#=========================================================================
# REMOVE SPACES, UNDERSCORES, OTHER NON ALPHANUMERIC CHARACTERS
#=========================================================================

proc filter_bad_chars {word} {
    set word [string tolower $word]
    regsub -all "\\." $word "_"  word    ;# eliminate '.' characters
    regsub -all "\\+" $word "p"  word    ;# eliminate '+' characters
    regsub -all "\\-" $word "_"  word    ;# eliminate '-' characters
    regsub -all {\[}  $word "_"  word    ;# eliminate '[' characters
    regsub -all {\]}  $word "_"  word    ;# eliminate ']' characters
    regsub -all " "   $word "_"  word    ;# eliminate ' ' characters
    regsub -all "#"   $word "_nr_"  word    ;# eliminate '#' characters
    return $word
}


#==========================================================================
# GENERATE NEW COLORS FOR GRAPHS
#==========================================================================
proc get_new_color { } {

    global color_pointer
    global max_color_number

    set color {navy blue cyan DarkGreen green yellow goldenrod sienna brown \
	    orange red maroon purple blue4}

    set max_color_number 13

    incr color_pointer

    if {$color_pointer > $max_color_number} {set color_pointer 0}

    return [lindex $color $color_pointer]

}

proc set_main_y_scale {new_ratio} {
    # this routine changes the effective y-scale from a the
    # standard value of 0-1 to a bigger range using the
    # value extracted rom the RHS slider
    global slider_scale
    global strip_chart

    if { $new_ratio<= 2 } {
	set mmax 1
	set mmin 0
    } else {
	set mmax $new_ratio
	set mmin [expr -1.* $new_ratio]
    }
    
    $strip_chart yaxis configure  -min $mmin -max $mmax
    update
    return
}
   

#===========================================================================
# DISPLAY HELP TEXT
#===========================================================================
proc help_menu { } {
    
    toplevel .info          ; # create a new window
    #  put a an <ok> button on it:
    button .info.ok -text "OK" -command {destroy .info ; return}
    pack   .info.ok -side bottom
    
    # now create the text widget
    # text .info.text -height 25 -width 65 
    # pack .info.text -side top
    
    #  put the text in the window:
    #  .info.text insert end   " \ 
    
    # use a message window instead of a text window - simpler
    message .info.mess -width 6.5i -text \
	    "Introduction: Stripchart can:\n\n\
	    1. plot any data in the\
	    midas database. It uses mchart, (a midas utility) to actually\
	    extract the data from odb\n\
	    2. view the data from mhist, the MIDAS history program.\n\n \
            This program should live in the ~/bin directory. To start it  \
	    type\n stripchart <name_of_conf_file> or\n stripchart -mhist\nto see\
	    data from the midas history\n\n \
	    The configuration files \
	    are generated by mchart (eg. target.conf, chvi.conf) and \
	    are in the same format as that understood by gnome \
            gstripchart.  \n\n \
	     Example: to see the CHAOS target info type: \n \
            mchart  -q /equipment/target/variables -f target.conf\n\
	    and then start\n stripchart target.conf\n or just type \n \
	    mchart  -q /equipment/target/variables -g -f target.conf \n \
	    \n\
	    Note that all graphs are scaled using the max/min values \
            defined in the .conf file to fit between 0 and 1. Hence the \
            unlabelled y-scale on the main window.\
	    To see a particular data set in its normal, unscaled \
            units, use 'view full graph' and select the line or just click \
	    with the mouse on a point close to the line of interest. \n\n \
	    Note that the single graphs can be zoomed by clicking the mouse, \
	    auto-scaled, or 'best' scaled. Hardcopy is available in ps, jpg, gif \
	    or png format and goes to a file.\n\
	    To use the history function, click on the mhist button. You must be \
	    in the directory containing the .hst (not the dra .hst) files. You will\
	    then be asked to select the event number and data works, as well as the\
	    history duration and interval\
	    \n\n \
	    G. Hofman    gertjan.hofman@colorado.edu \n \
	    March-00. \n \
	    CU - Boulder"

    pack .info.mess -side bottom
    
    update
    return
}




#======================================================================================
#                                    MAIN PROGRAM 
#======================================================================================

# set a few parameters

set color_pointer 0

set tit_font "-*-helvetica-bold-r-normal-*-13-180-*-*-*-*-*-*"

# define the default scrolling time period
set time_limit [expr 60*60]               ;# in seconds 5 min default.

# default y-scaling  values for the pop-up windows
set scale_ymin ""
set scale_ymax "" 

set last_rescale_time  [clock seconds]  ;# last time  we checked the scaling
                                         # of the pop up windows
set conf_fname ""
set equip_name ""
set item_list  ""

set doing_mhist 0                       ;# data from mhist OR mchart conf file
set debug ""

# get the file name of the .conf files. Command line parsing
set arguse 0               ;# no arguments used up by options

if {$argc!=0} {
    for {set i 0} {$i < $argc } {incr i} {
	switch -glob -- [lindex  $argv  $i] {
	    "-mhist"       { set doing_mhist 1 ; incr arguse}
	    "-debugmhist"  { set debug mhist   ; incr arguse}
	}
    }
    if {$arguse < $argc} { set conf_fname [lindex $argv [expr $argc -1] ]}
} else {
    puts "usage:  stripchart <-mhist> <confilename.conf> "
    exit
}


if {!$doing_mhist} {
    if {![file exists $conf_fname]} {
	tk_messageBox  -message "Error: conf  file $conf_fname not found_!" -type ok
	exit
    } else {
	# read in the .conf file and parse:
	read_conf_file $conf_fname
    }
}


frame .main  -background  beige               ;# define name of main window.
frame .main.toprow   -relief ridge -bd 3      ;# for the buttons
frame .main.middle   -relief ridge -bd 3      ;# for the graph
frame .main.botrow                            ;# unused for now

wm title  . "midas: $equip_name"              ;# configure the FVWM window
wm iconname . "StripC"

set strip_chart .main.middle.strip_chart

graph $strip_chart -background goldenrod  -title ""                 ;# no title on the graph.

$strip_chart configure -width 5.5i -height 2.0i -font $tit_font

#$strip_chart marker create bitmap -under yes -coords { .2 .7}  -bitmap @midas.xbm


# configure the x-axis to plot Clock time instead of plain numbers
$strip_chart xaxis configure -loose 0 -title ""  \
	-command "my_clock_format %H:%M" -tickfont $tit_font -titlefont $tit_font

# fix the yaxis scale
$strip_chart yaxis configure -min 0 -max 1 \
	-tickfont $tit_font -titlefont $tit_font -showticks 0

# switch on cross hairs
#Blt_Crosshairs $strip_chart      ;# what is this comand ???
# bind the mouse click to the item search routine and pass 
# window name, x and y coordinate
bind $strip_chart <B1-ButtonRelease> "mouse_find_item %W %x %y"

# create an exit button. The -bd 0 mean a tight fit of text - no border
button  .main.toprow.exit -text "exit" -command {nice_exit} -relief flat -underline 0 
pack .main.toprow.exit -side left


# create an exit button
button  .main.toprow.help -text "help" -command {help_menu} -relief flat -underline 0 -bd 0
pack .main.toprow.help -side left


# menu for selecting of individual items = make a 'menubutton'

set  select_but [menubutton  .main.toprow.select -text "detail-graph" \
        -menu .main.toprow.select.mnu  -relief flat -underline 0 -bd 0 ]
set  select_men [menu $select_but.mnu]

pack $select_but -side left -fill x

# create the pull down item menu. Also align the text string
set blank_string "                            "
foreach item $item_list {
    set item_length [string length $item]
    if {$item_length > 13} {set item_length 13}
    set menu_string "$item [string range $blank_string 1 [expr 13 - $item_length] ]"
    append menu_string $item_color($item)
    $select_men add command -command "select_graph $item" -label  $menu_string
}

# create an *new* scroll time selection. 

menubutton .main.toprow.scrollt -text "scroll time" -menu .main.toprow.scrollt.mnu \
	-relief flat -underline 0 -bd 0
menu .main.toprow.scrollt.mnu 
.main.toprow.scrollt.mnu  add radiobutton -label "100 s"    -variable  time_limit \
	-value 100
.main.toprow.scrollt.mnu  add radiobutton -label "5 mins"   -variable  time_limit \
	-value 300 
.main.toprow.scrollt.mnu  add radiobutton -label "30 mins"  -variable  time_limit \
	-value [expr 30*60] 
.main.toprow.scrollt.mnu  add radiobutton -label "1 hour"   -variable  time_limit \
	-value [expr 60*60]
.main.toprow.scrollt.mnu  add radiobutton -label "10 hours" -variable  time_limit \
	-value [expr 10*60*60] 
.main.toprow.scrollt.mnu  add radiobutton -label "24 hours" -variable  time_limit \
	-value [expr 24*60*60]
pack  .main.toprow.scrollt  -side left -fill x

# old button with callback to old routine.
#button  .main.toprow.scroll2 -text "scroll time" -command {new_scroll_time} \
#	-relief flat -underline 0 -bd 0
#pack .main.toprow.scroll2 -side left 

button .main.toprow.hist -text "mhist" -command {read_mhist}  \
	-relief flat -underline 0 -bd 0
pack .main.toprow.hist -side left


# create a warning label to show whether we have new  data coming in
label .main.toprow.update -relief sunken   \
	-textvariable prev_update_time_formatted -bd 0
pack .main.toprow.update -side right
label  .main.toprow.warning -relief flat -pady 4  \
	-text "Update: " -background green -bd 0
pack .main.toprow.warning -side right -fill x

# pack all the remaining windows: - first pack graph in middle row:
# note: its the 'fill both' and 'expand 1' that does the resizing
# of the graph when the window size changes.

pack $strip_chart -side left  -fill both -expand 1  ;# put it in its parent.

# then pack middlerow in the .main:
pack .main.middle -side top -fill both -expand 1 
pack .main.toprow -side top -fill x    -expand 1

#pack .main.botrow -side top -fill both -expand 1 ; # bot row unused
 
pack .main -fill both -expand 1                   ;# put it all on the screen
update

# try the slider to change the scale of the main graph
# note: the -command automatically appends the value of the slide
#set scale_slider 1
#scale .main.middle.slide -orient vertical  -label ""  -variable scale_slider \
#	-borderwidth 0  -showvalue 0 -width 10 -from 10 -to .1 \
#	-command "set_main_y_scale"
#pack .main.middle.slide -side right -fill y -expand 1

# ok, finally, create vectors associated with the data.
# create also the graph elements:

foreach item $item_list {
    if {! [$strip_chart element exists line_$item]} {
	$strip_chart element create line_$item
    } else {
	tk_messageBox -message "Error(online): line $item already exist - skipped "
    }
    $strip_chart element configure line_$item -label ""  -color $item_color($item)  -symbol ""
    vector create V_y_$item                ;# for storing raw data
    vector create V_y_plot_$item           ;# 'scaled' vector for plotting
    vector create V_x_$item                ;# time vector.
    # now hot-link it to the new graph line 
    $strip_chart element configure line_$item  -xdata V_x_$item  -ydata V_y_plot_$item
}

update


# ok, done all the things we can do for both mhist AND normal stripcharting
# if we didn't open a .conf file, just sit here and wait:
if {$doing_mhist} {
    .main.toprow.warning configure -text "mhist " -background  cyan 
    wm title  . "mhist: g.j hofman Mar-00 test version"  
    while {1} {
	wait_ms 200
    }
}

    

# ===============================================================================
# CODE BELOW FOR ONLINE STRIPCHARTING ONLY
# ===============================================================================
# for now I am assuming all data values are read from the SAME file.

set data_fname  $item_fname([lindex $item_list 0])   ;# just grab the first filename


# main plotting loop:

set prev_update_time  0            ;# last time the data file was written

while {1} {

# check if the file exist and when it was last written

    if {![file exists $data_fname]} {
        show_message "Error: data  file $data_fname not found !"
	.main.toprow.warning configure -text "No Data File" -background  red 
	vwait message_done         ;# wait until message ok button was hit.
	# go into loop, waiting for file:
	while {![file exists $data_fname]} {
	    wait_ms 1000
	}
    } else {
	
	# check the last modification time of the file:
	while { [file mtime $data_fname]== $prev_update_time} {
	    after 100
	    update
	    set time_passed [expr ( [clock seconds] - [file mtime $data_fname])]
	    if { $time_passed > 200 } { 
		.main.toprow.warning configure -text "No Data" -background  red 
	    }   
	}
    }

# ok, we have a new file:
    .main.toprow.warning configure -text "Update: "  -background green

    set prev_update_time [file mtime $data_fname]
# for output to screen, format the time for humans:
    set prev_update_time_formatted  [clock format $prev_update_time -format "%H:%M:%S"]
    
    set file_hndl [open $data_fname r]
    
    # loop over all lines in the file
    while   {! [eof $file_hndl]} {
	
	gets $file_hndl string
	set item_string [lindex $string 0]
	
	set item_string [string tolower $item_string]     
	# remove underscores etc.
	set item_string [filter_bad_chars $item_string]

	# loop over all known keyword and compare to file.

	foreach item $item_list {
	    if { [string first $item $item_string] != -1} {

		set new_data  [expr 1.0 *[lindex $string 1]]
		set vec_name  V_x_$item
		set ${vec_name}(++end)  $prev_update_time
		set vec_name V_y_$item
		set ${vec_name}(++end)  $new_data

		# now scale the vector to fit on the graph.
		set plot_vec_name  V_y_plot_$item
		set norm_data \
		[expr " ($new_data -  $item_min($item)) / ($item_max($item)-$item_min($item)) " ] 
		set ${plot_vec_name}(++end) $norm_data
	    }
	} 
    }

    close $file_hndl


    # take care of the scrolling:
    # I do some tricky double de-referencing here. I need to access the
    # vector but I only have the name of the vector. So the inner 'set'
    # returns the name of the vector. I then concatenate (0) and 
    # finally get the actual value stored in the vector


    foreach item $item_list {
	set oldest_time [ set V_x_[set  item](0)]
	set newest_time [ set V_x_[set  item](end)]

#	puts "data are now [set V_y_[set item](:)]"

	if {  [expr ( $newest_time - $oldest_time)] > $time_limit } {
	    V_x_$item delete 0
	    V_y_$item delete 0
	    V_y_plot_$item delete 0
	}
    }


    # also check if we you re-evaluate the scaling on the individual plots
    # say at every 300 s.
    set time_now [clock seconds]
    if { [expr $time_now - $last_rescale_time ] > 300} {
	foreach item $item_list {
	    if  [winfo exists .fullscale$item ] {
		scale_single_window Rescale $item
	    }
	}
	set last_rescale_time $time_now
    }

    update

    wait_ms 2000



}



#=======================================================================================
#====================== END OF CODE ====================================================
#=======================================================================================