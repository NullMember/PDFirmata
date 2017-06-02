; Script generated by the HM NIS Edit Script Wizard.
; Manually edited by Roman Haefeli
; the string PDVERSION should be filled in (e.g., to 0.46-7)

; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "Pure Data"
!define PRODUCT_VERSION "PDVERSION"
!define PRODUCT_PUBLISHER "Miller Puckette"
!define PRODUCT_WEB_SITE "http://www.puredata.info"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\pd.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

# Components texts
!define COMPONENT_GROUP_TEXT "${PRODUCT_NAME}"
!define COMPONENT_MAIN_TEXT "Application"
!define COMPONENT_STARTMENU_TEXT "Create Startmenu entry"
!define COMPONENT_DESKTOPSHORTCUT_TEXT "Create Desktop Shortcut"
!define	COMPONENT_FILEASSOC_TEXT "Open .pd-files with Pd"

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"
!define MUI_COMPONENTSPAGE_NODESC

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!include "/tmp/license_page.nsh"
; Components page
!insertmacro MUI_PAGE_COMPONENTS
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; set file associations page

; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\bin\pd.exe"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\doc\1.manual\index.htm"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; MUI end ------

; Function to refresh shell icons
!define SHCNE_ASSOCCHANGED 0x08000000
!define SHCNF_IDLIST 0

Function RefreshShellIcons
  ; By jerome tremblay - april 2003
  System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v \
  (${SHCNE_ASSOCCHANGED}, ${SHCNF_IDLIST}, 0, 0)'
FunctionEnd
; /Function

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "/tmp/pd-${PRODUCT_VERSION}.windows-installer.exe"
InstallDir "$PROGRAMFILES\Pd"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

SectionGroup /e "${COMPONENT_GROUP_TEXT}"
  Section "${COMPONENT_MAIN_TEXT}" PureData
    SectionIn RO
    !include "/tmp/install_files_list.nsh"
  SectionEnd

  Section "${COMPONENT_STARTMENU_TEXT}" StartMenu
    SetOutPath $INSTDIR
    WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
    CreateDirectory "$SMPROGRAMS\Pd"
    CreateShortCut "$SMPROGRAMS\Pd\Website.lnk" "$INSTDIR\${PRODUCT_NAME}.url"
    CreateShortCut "$SMPROGRAMS\Pd\Uninstall.lnk" "$INSTDIR\uninst.exe"
    CreateShortCut "$SMPROGRAMS\Pd\Pd.lnk" "$INSTDIR\bin\pd.exe"
  SectionEnd

  Section "${COMPONENT_DESKTOPSHORTCUT_TEXT}" DesktopShortcut
    CreateShortCut "$Desktop\Pd.lnk" "$INSTDIR\bin\pd.exe"
  SectionEnd

  Section "${COMPONENT_FILEASSOC_TEXT}" SetFileAssociations
  ; Set file ext associations
    WriteRegStr HKCR ".pd" "" "PureData"
    WriteRegStr HKCR "PureData" "" ""
    WriteRegStr HKCR "PureData\shell" "" ""
    WriteRegStr HKCR "PureData\shell\open" "" ""
    WriteRegStr HKCR "PureData\shell\open\command" "" "$INSTDIR\bin\pd.exe %1"
  ; Set file ext icon
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.pd" "" ""
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.pd\OpenWithList" "a" "pd.exe"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.pd\OpenWithList" "MRUList" ""
    WriteRegBin HKCU "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.pd\OpenWithProgids" "PureData" "0"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.pd\UserChoice" "Progid" "PureData"
  ; Refresh Shell Icons
    Call RefreshShellIcons
  SectionEnd
SectionGroupEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\bin\pd.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\bin\pd.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd

Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort
FunctionEnd

Section Uninstall
  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\uninst.exe"
  Delete "$SMPROGRAMS\Pd\Uninstall.lnk"
  Delete "$SMPROGRAMS\Pd\Website.lnk"
  Delete "$SMPROGRAMS\Pd\Pd.lnk"
  RMDir "$SMPROGRAMS\Pd"
  Delete "$DESKTOP\Pd.lnk"

  !include "/tmp/uninstall_files_list.nsh"

; file ext association
  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  DeleteRegKey HKCR ".pd"
  DeleteRegKey HKCR "PureData"

; file ext icon
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.pd"

  SetAutoClose true
SectionEnd
