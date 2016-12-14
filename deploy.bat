
mkdir tmp\speedreader
copy .\Deploy\Speedreader.exe .\tmp\speedreader
mkdir .\tmp\speedreader\Engine
xcopy '.\Speedreader\Engine\dll(x86)' .\tmp\speedreader\Engine
copy .\Speedreader\speedreader.ini .\tmp\speedreader
xcopy .\Speedreader\speedreader\doc .\tmp\speedreader
cd tmp
"C:\Program Files\7-Zip\7z.exe" a -r speedreader.zip speedreader
copy speedreader.zip c:\Users\nishio\Desktop



