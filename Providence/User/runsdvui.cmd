cd /d "c:\Users\Zen\documents\visual studio 2015\Projects\Providence\User" &msbuild "User.vcxproj" /t:sdvViewer /p:configuration="Debug" /p:platform=Win32
exit %errorlevel% 