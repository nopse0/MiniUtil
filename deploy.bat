set root="C:\Build\MiniUtil\MiniUtil"
set dest="D:\Games\Skyrim-1.6.1170-shattered"
set pluginsrc=%root%\build\release-msvc
set plugindest=%dest%\overwrite
copy %pluginsrc%\*.dll %plugindest%\SKSE\Plugins\
copy %pluginsrc%\*.pdb %plugindest%\SKSE\Plugins\
