; elog.nsi
;
; This script will install the elog system
;

; The name of the installer
Name "ELOG"

; The file to write
OutFile "elog${VERSION}.exe"

; The default installation directory
InstallDir $PROGRAMFILES\ELOG
; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM SOFTWARE\ELOG "Install_Dir"

; The text to prompt the user to enter a directory
ComponentText "This will install the ELOG electronic logbook server on your computer. Select which optional things you want installed."
; The text to prompt the user to enter a directory
DirText "Choose a directory to install in to:"

; Main system
Section "ELOG system (required)"
  ; root directory
  SetOutPath $INSTDIR
  File "COPYING"
  File /oname=CHANGELOG.TXT doc\CHANGELOG_ELOG.TXT 
  File utils\elog*.c
  File nt\bin\elog*.exe
  File /oname=elogd.cfg c:\elog\elogd_sample.cfg 
  File c:\elog\eloghelp.html
  
  File /oname=readme.html m:\html\elog\index.html
  File m:\html\elog\config.html
  File m:\html\elog\faq.html
  File m:\html\elog\*.gif
  File m:\html\elog\*.jpg
  
  SetOutPath $INSTDIR\default
  File c:\elog\default\*.gif
  File c:\elog\default\*.cfg

  SetOutPath $INSTDIR\linux
  File c:\elog\011108.log
    
  SetOutPath $INSTDIR
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\ELOG "Install_Dir" "$INSTDIR"
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ELOG" "DisplayName" "ELOG electronic logbook (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ELOG" "UninstallString" '"$INSTDIR\uninst_elog.exe"'
SectionEnd

; optional section
Section "Start Menu Shortcuts"
  CreateDirectory "$SMPROGRAMS\ELOG"
  CreateShortCut "$SMPROGRAMS\ELOG\ELOG server.lnk" "$INSTDIR\elogd.exe" "" "$INSTDIR\elogd.exe" 0 
  Delete "$SMPROGRAMS\ELOG\Demo Logbook.lnk"
  WriteINIStr "$SMPROGRAMS\ELOG\Demo Logbook.url" \
              "InternetShortcut" "URL" "http://localhost/linux/"
  CreateShortCut "$SMPROGRAMS\ELOG\ELOG Documentation.lnk" "$INSTDIR\readme.html"
  CreateShortCut "$SMPROGRAMS\ELOG\Uninstall ELOG.lnk" "$INSTDIR\uninst_elog.exe" "" "$INSTDIR\uninst_elog.exe" 0
SectionEnd

; display readme file

Function .onInstSuccess
  MessageBox MB_YESNO|MB_ICONQUESTION \
             "Setup has completed. View readme file now?" \
             IDNO NoReadme
    ExecShell open '$INSTDIR\readme.html'
  NoReadme:
FunctionEnd

; uninstall stuff

UninstallText "This will uninstall ELOG."
UninstallExeName "uninst_elog.exe"

; special uninstall section.
Section "Uninstall"
  ; remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ELOG"
  DeleteRegKey HKLM SOFTWARE\ELOG

  ; remove files
  Delete $INSTDIR\COPYING
  Delete $INSTDIR\CHANGELOG.TXT
  Delete $INSTDIR\elog.c
  Delete $INSTDIR\elogd.c
  Delete $INSTDIR\elog.exe
  Delete $INSTDIR\elogd.exe
  Delete $INSTDIR\elogd.cfg
  Delete $INSTDIR\readme.html
  Delete $INSTDIR\*.gif
  Delete $INSTDIR\*.jpg
  
  Delete $INSTDIR\default\*.*
  Delete $INSTDIR\linux\*.*
  RMDir $INSTDIR\default
  RMDir $INSTDIR\linux
  
  ; MUST REMOVE UNINSTALLER, too
  Delete $INSTDIR\uninst_elog.exe

  ; remove shortcuts, if any.
  Delete "$SMPROGRAMS\ELOG\*.*"

  ; remove directories used.
  RMDir "$SMPROGRAMS\ELOG"
  RMDir "$INSTDIR"

  ; if $INSTDIR was removed, skip these next ones
  IfFileExists $INSTDIR 0 Removed 
    MessageBox MB_YESNO|MB_ICONQUESTION \
      "Remove all files in your ELOG directory? (If you have anything\
 you created that you want to keep, click No)" IDNO Removed
    Delete $INSTDIR\*.* ; this would be skipped if the user hits no
    RMDir /r $INSTDIR
    IfFileExists $INSTDIR 0 Removed 
      MessageBox MB_OK|MB_ICONEXCLAMATION \
                 "Note: $INSTDIR could not be removed."
  Removed:

SectionEnd

; eof
