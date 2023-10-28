<?php
$hpp_fh = fopen("bin.hpp", "w");
fwrite($hpp_fh, "#pragma once\r\n\r\nstruct bin\r\n{");
$cpp_fh = fopen("bin.cpp", "w");
fwrite($cpp_fh, "#include \"bin.hpp\"\r\n\r\n");
foreach([
	"logitech_liaison" => __DIR__."/../logitech-liaison.exe",
	"LogitechLed_x64" => __DIR__."/LogitechLed_x64.dll",
	"LogitechLed_x86" => __DIR__."/LogitechLed_x86.dll",
] as $name => $path)
{
	$bin_str = file_get_contents($path);
	fwrite($hpp_fh, "\r\n\tstatic const char {$name}[".strlen($bin_str)."];");
	fwrite($cpp_fh, "const char bin::{$name}[] = { '\\x".join("', '\\x", array_map("dechex", array_map("ord", str_split($bin_str))))."' };\r\n");
}
fwrite($hpp_fh, "\r\n};\r\n");
fclose($hpp_fh);
fclose($cpp_fh);
