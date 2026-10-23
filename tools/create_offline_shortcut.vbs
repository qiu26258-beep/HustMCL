' Create a per-machine desktop shortcut for the deployed HustMCL offline package.
' This file is copied next to hustmcl_offline.exe. It intentionally uses only
' the directory containing this script, never a path from the build machine.
Option Explicit

Const APP_EXE = "hustmcl_offline.exe"
Const PRIMARY_LINK = "HustMCL.lnk"
Const FALLBACK_LINK = "HustMCL Offline 26.2.lnk"

Dim fso
Dim shell
Dim packageDir
Dim exePath
Dim desktopDir
Dim linkPath

Set fso = CreateObject("Scripting.FileSystemObject")
Set shell = CreateObject("WScript.Shell")

packageDir = fso.GetParentFolderName(WScript.ScriptFullName)
exePath = fso.BuildPath(packageDir, APP_EXE)

If Not fso.FileExists(exePath) Then
    WScript.Quit 2
End If

desktopDir = shell.SpecialFolders("Desktop")
If desktopDir = "" Then
    WScript.Quit 3
End If
If Not fso.FolderExists(desktopDir) Then
    WScript.Quit 3
End If

linkPath = fso.BuildPath(desktopDir, PRIMARY_LINK)

If fso.FileExists(linkPath) Then
    If ShortcutTargetsPackage(linkPath, exePath) Then
        UpdateShortcut linkPath, exePath, packageDir
        WScript.Quit 0
    End If

    ' A different program owns HustMCL.lnk. Keep it untouched and use a
    ' package-specific name only when that name is free or belongs to us.
    linkPath = fso.BuildPath(desktopDir, FALLBACK_LINK)
    If fso.FileExists(linkPath) Then
        If Not ShortcutTargetsPackage(linkPath, exePath) Then
            WScript.Quit 4
        End If
    End If
End If

UpdateShortcut linkPath, exePath, packageDir
WScript.Quit 0

Sub UpdateShortcut(ByVal shortcutPath, ByVal targetPath, ByVal workDir)
    Dim shortcut
    Set shortcut = shell.CreateShortcut(shortcutPath)
    shortcut.TargetPath = targetPath
    shortcut.WorkingDirectory = workDir
    shortcut.IconLocation = targetPath & ",0"
    shortcut.Description = "HustMCL Offline 26.2"
    shortcut.WindowStyle = 1
    shortcut.Save
End Sub

Function ShortcutTargetsPackage(ByVal shortcutPath, ByVal expectedExe)
    Dim shortcut
    Dim actualTarget
    Dim resolvedTarget
    Dim resolvedExpected

    On Error Resume Next
    Set shortcut = shell.CreateShortcut(shortcutPath)
    actualTarget = shortcut.TargetPath
    If Err.Number <> 0 Then
        Err.Clear
        On Error GoTo 0
        ShortcutTargetsPackage = False
        Exit Function
    End If
    On Error GoTo 0

    If Len(Trim(actualTarget)) = 0 Then
        ShortcutTargetsPackage = False
        Exit Function
    End If

    On Error Resume Next
    resolvedTarget = fso.GetAbsolutePathName(actualTarget)
    resolvedExpected = fso.GetAbsolutePathName(expectedExe)
    If Err.Number <> 0 Then
        Err.Clear
        On Error GoTo 0
        ShortcutTargetsPackage = False
        Exit Function
    End If
    On Error GoTo 0

    resolvedTarget = Replace(resolvedTarget, "/", "\")
    resolvedExpected = Replace(resolvedExpected, "/", "\")
    If Right(resolvedTarget, 1) = "\" Then
        resolvedTarget = Left(resolvedTarget, Len(resolvedTarget) - 1)
    End If
    If Right(resolvedExpected, 1) = "\" Then
        resolvedExpected = Left(resolvedExpected, Len(resolvedExpected) - 1)
    End If

    ShortcutTargetsPackage = (StrComp(resolvedTarget, resolvedExpected, vbTextCompare) = 0)
End Function
