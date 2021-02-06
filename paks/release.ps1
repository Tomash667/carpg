Get-Content "../source/Version.h" | ForEach-Object {
    if($_.StartsWith("#define VERSION_MAJOR")) {
        $str = $_.Split(" ")
        $major = $str[2]
    }
    elseif($_.StartsWith("#define VERSION_MINOR")) {
        $str = $_.Split(" ")
        $minor = $str[2]
    }
    elseif($_.StartsWith("#define VERSION_PATCH")) {
        $str = $_.Split(" ")
        $path = $str[2]
    }
}
if($path -eq 0) {
    $ver = "$major.$minor"
} else {
    $ver = "$major.$minor.$path";
}

Echo "Current version: $ver"

if($path -eq 0) {
    $confirm = Read-Host "Full version and path will be generated. Are you sure? (y/n)"
    if($confirm -eq 'y') {
        .\paker.exe -both -blob $ver | Write-Host
        Echo "Done"
    }
}
else {
    $confirm = Read-Host "Path will be generated. Are you sure? (y/n)"
    if($confirm -eq 'y') {
        .\paker.exe -patch -blob $ver | Write-Host
        Echo "Done"
    }
}
