var=$1
tmp=${var##*/}
outname=${tmp%.*}
xxd -i $var > $outname.c
