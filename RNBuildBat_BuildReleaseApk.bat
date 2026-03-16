::@echo off
::echo param[1] = %1
if "%2%" == "yes" (
	echo need clean
	call RNBuildBat_Clean_do.bat
	cd ..
) else (
	echo no clean
)

::echo param[3] = %3
if "%4%" == "yes" (
	echo need bundle
	call RNBuildBat_BundleJs.bat
) else (
	echo no bundle
)

call RNBuildBat_RemoveTempFiles.bat
cd ../../../../..

call RNBuildBat_BuildReleaseApk_do.bat
cd ..