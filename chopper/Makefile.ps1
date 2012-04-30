$env:SCALE = 0.5
$env:FUSELAGE_DB = "../data/comanche.db"
$env:FUSELAGE_ASSETS = "fuselage*.g"
$env:ROTOR_DB = "../data/rotor.db"
$env:ROTOR_ASSETS = "rotor.g"

$files=get-childitem -Filter *.obj 
$filelist = ""
foreach ($file in $files) {
	Start-Process -Wait -FilePath "..\obj2asset.py" -ArgumentList ($file.BaseName + ".obj" + " " + $file.BaseName + ".g")
	$filelist = ($filelist + " " + $file.BaseName + ".g")
}

$gfiles=get-childitem -Filter $env:FUSELAGE_ASSETS 
$gfilelist = ""
foreach ($file in $gfiles) {
	$gfilelist = ($gfilelist + " " + $file.BaseName + ".g")
}

Start-Process -Wait -FilePath "..\spankassets" -ArgumentList "$env:FUSELAGE_DB $gfilelist"
																																	 

$gfiles=get-childitem -Filter $env:ROTOR_ASSETS
$gfilelist = ""
foreach ($file in $gfiles) {
	$gfilelist = ($gfilelist + " " + $file.BaseName + ".g")
}

Start-Process -Wait -FilePath "..\spankassets" -ArgumentList "$env:ROTOR_DB $gfilelist"
																																	 
																																	