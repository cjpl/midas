#!/usr/bin/perl
# above is magic first line to invoke perl
#
use warnings;
use strict;
##
# 
my $writefile = "index_add.txt";
open OUTD, ">$writefile" or die "Can't open output file $writefile : $!\n";

print OUTD<<EOT;
# index_add.txt 
#   
#                  This file produced by index_add.pl
#
#  DO NOT EDIT index_add.txt.  ADD NEW ITEMS TO index_add.pl
#
# these items will be added to the index by index.pl
#
# used to add "see" links in the index without putting more anchors into the text
#
# Links are within the index (docindex.dox) to e.g. IDX_item
#       (IDX anchors automatically created for level 1 items only)
#                            
#          IDX=item in this file will be substituted to IDX_item (by check_line in index.pl)
#   dashes will be substituted to spaces; can leave spaces rather than dashes in this file
#
# e.g
# 
# mhttpd_links-see-\@ref IDX=mhttpd "mhttpd buttons" will become in docindex.dox
# ...
#     <li>links <span class="see">see</span> \@ref IDX_mhttpd "mhttpd buttons"<br> <!-- line from index_add.txt -->
#
#  mhttpd
#        links see mhttpd buttons
#
#
# List of links
equipment_page-see-\@ref IDX=mhttpd "mhttpd->page->equipment"
status_page-see-\@ref IDX=mhttpd "mhttpd->page->status"
alarm_page-see-\@ref IDX=mhttpd "mhttpd->page->alarm"
alias_page-see-\@ref IDX=mhttpd "mhttpd->page->alias"
CNAF_page-see-\@ref IDX=mhttpd "mhttpd->page->CNAF"
custom_page-see-\@ref IDX=mhttpd "mhttpd->page->custom"
history_page-see-\@ref IDX=mhttpd "mhttpd->page->history"
message_page-see-\@ref IDX=mhttpd "mhttpd->page->message"
MSCB_page-see-\@ref IDX=mhttpd "mhttpd->page->MSCB"
ODB_page-see-\@ref IDX=mhttpd "mhttpd->page->ODB"
elog_page-see-\@ref IDX=mhttpd "mhttpd->page->elog"
program_page-see-\@ref IDX=mhttpd "mhttpd->page->program"
slow-control_page-see-\@ref IDX=mhttpd "mhttpd->page->slow-control"
start_page-see-\@ref IDX=mhttpd "mhttpd->page->start"
buttons-see-\@ref IDX=mhttpd "mhttpd buttons"
mhttpd_links-see-\@ref IDX=mhttpd "mhttpd buttons"
alias_buttons/links-see-\@ref IDX=mhttpd "mhttpd"
shift-check_button-see-\@ref IDX=mhttpd "mhttpd->page->elog"
event_equipment_flags-see-\@ref IDX=equipment "equipment->flags"
user_buttons/links-see-\@ref IDX=mhttpd "mhttpd->buttons->script"
script_buttons/links-see-\@ref IDX=mhttpd "mhttpd->buttons->script"
custom_buttons/links-see-\@ref IDX=mhttpd "mhttpd"
edit_boxes-see-\@ref IDX=mhttpd "mhttpd"
mhttpd_customscript-see-\@ref IDX=mhttpd "mhttpd->buttons->custom"
mhttpd_custom-see-\@ref IDX=mhttpd "mhttpd->page->custom"
midas_logger-see-\@ref IDX=mlogger "mlogger"
format-see-also-\@ref IDX=event "event->format"
state-see-\@ref IDX=run "run->state"
buffer-see-also \@ref IDX=event "event->buffer"
event-builder-see-\@ref-IDX=event-"event->builder"
fragmented-event-see-\@ref IDX=event "event->fragmented"
bank-see-\@ref IDX=event "event->readout"
data_bank-see-\@ref IDX=event "event->readout"
equipment_readout_flags-see-\@ref IDX=event "event->readout->flags"
experiment_define-see-\@ref IDX=exptab "exptab"
programs_ODB tree-see-\@ref IDX=ODB "ODB->tree->programs"
script_ODB tree-see-\@ref IDX=ODB "ODB->tree->script"
customscript_ODB tree-see-\@ref IDX=ODB "ODB->tree->customscript"
runinfo_ODB-tree-see-\@ref IDX=ODB "ODB->tree->runinfo"
system_ODB-tree-see-\@ref IDX=ODB "ODB->tree->system"
logger_ODB-tree-see-\@ref IDX=ODB "ODB->tree->logger"
equipment_ODB-tree-see-\@ref IDX=ODB "ODB->tree->equipment"
custom_ODB-tree-see-\@ref IDX=ODB "ODB->tree->custom"
alias_ODB-tree-see-\@ref IDX=ODB "ODB->tree->alias"
alarm_ODB-tree-see-\@ref IDX=ODB "ODB->tree->alarms"
Online-Data-Base-see-\@ref IDX=ODB "ODB"
midas_remote-server-see-\@ref IDX=mserver "mserver"
experiment_access-control-see-\@ref IDX=access "access control"
read-on_flags-see-\@ref IDX=event "event->readout->flags"
RO_flags-see-\@ref IDX=event "event->readout->flags"
EQ_flags-see-\@ref IDX=equipment "equipment->flags"
equipment_readout_flags-see-\@ref IDX=event "event->readout->flags"
transitions_event_readout-see-\@ref IDX=event "event->readout->flags"
run_state_event_readout-see-\@ref IDX=event "event->readout->flags"
event-see-also-\@ref-IDX=equipment "equipment"
tasks-see-\@ref IDX=utilities "utilities"
logger-see-also-\@ref IDX=Logging "Logging"
logging_ODB-contents-see-\@ref IDX=ODB "ODB->Dump"
compression-see-\@ref IDX=logger "Logging->Data->Format->compression"
compression_building-option-see-\@ref IDX=ZLIB "ZLIB"
clients-see-name-of-individual-client
equipment-Names-array-see-\@ref IDX=mhttpd "mhttpd->page->equipment"
tags-see-\@ref IDX=history "history->tags"
data_logger_utility-see-\@ref IDX=mlogger "mlogger"
data_logging-see-\@ref IDX=logger "logging"
data_format-see-\@ref IDX=format
ODBC_building-option-see-\@ref IDX=ODBC "ODBC"
SQL-see-\@ref IDX=mySQL "mySQL"
data_analyzer-see-\@ref IDX=analyzer "analyzer"
ROME-see-\@ref IDX=analyzer "analyzer->ROME"
MIDAS_analyzer-see-\@ref IDX=analyzer "analyzer->MIDAS"
ROOT_analyzer-see-\@ref IDX=analyzer "analyzer->Rootana"
Rootana-see-\@ref IDX=analyzer "analyzer->Rootana"
Roody-see-\@ref IDX=analyzer "analyzer->Roody"
driver-see-\@ref IDX=device "device->driver"
midas_log-file-see-\@ref IDX=midas "message->log-file"
midas.log-see-\@ref  IDX=midas "message->log-file"
CAMAC-see-\@ref IDX=hardware "hardware"
VME-see-\@ref IDX=hardware "hardware"
GPIB-see-\@ref IDX=hardware "hardware"
USB-see-\@ref IDX=hardware "hardware"
Equipment_hardware-see-\@ref IDX=hardware "hardware"
hardware_supported-see-\@ref IDX=hardware "hardware->driver->library"
security-see-\@ref IDX=access "access control"
manual-trigger-see-also-\@ref IDX=event "event->manual-trigger"
#
# DO NOT EDIT index_add.txt
# 
#         Add new items to index_add.pl
EOT
