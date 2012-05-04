$env:SCALE = 1.0
$env:DB = "../data/assets.db"

$files=get-childitem -Filter *.obj 
$filelist = ""
foreach ($file in $files) {
	Start-Process -Wait -FilePath "..\obj2asset.py" -ArgumentList ($file.BaseName + ".obj")
	$filelist = ($filelist + " " + $file.BaseName + ".g")
}

$gfiles=get-childitem -Filter *.g 
$gfilelist = ""
foreach ($file in $gfiles) {
	$gfilelist = ($gfilelist + " " + $file.BaseName + ".g")
}

Start-Process -Wait -FilePath "..\spankassets" -ArgumentList "$env:DB $gfilelist"
