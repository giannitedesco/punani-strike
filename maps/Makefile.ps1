$env:DEST = "../data/maps/"
$env:BUILDER = "..\mkmap.exe"
$env:EXTENSION = ".m"
$env:SOURCEFILES = ( "*" + $env:EXTENSION ) 

if ((Test-Path -path $env:DEST) -ne $True) {
	New-Item -type directory $env:DEST 
}

$files=get-childitem -Filter $env:SOURCEFILES
foreach ($file in $files) {
	Start-Process -Wait -FilePath $env:BUILDER -ArgumentList ($env:DEST + $file.BaseName + " " + $file.BaseName + $env:EXTENSION)
}
