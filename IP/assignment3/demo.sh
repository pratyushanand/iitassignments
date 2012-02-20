./encoding -i lena.tif -o encoded.pat -l 2
./decoding -i encoded.pat -o decoded.tif
./video_encoding -i umcp.mpg -o compressed.mpat
-i compressed.mpat -o uncompressed.mpg
