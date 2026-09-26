set root="C:\Build\MiniUtil\MiniUtil"
set dest="D:\Games\Skyrim-1.6.1170-shattered"
set pluginsrc=%root%\build\release-msvc
set plugindest=%dest%\overwrite
set inisrc=%root%\config
copy %pluginsrc%\*.dll %plugindest%\SKSE\Plugins\
copy %pluginsrc%\*.pdb %plugindest%\SKSE\Plugins\
copy %inisrc%\*.ini %plugindest%\SKSE\Plugins\
