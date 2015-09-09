# mp3check #

Check mp3 files for consistency and print several errors and warnings. List stream attributes, highlighting deviations from 'normal' mp3 files. Layer 1,2,3, mpeg1.0+2.0 are currently supported. CRC check for layer 3. Can process directories recursively. Can also fix certain errors. Can strip valid and invalid headers/trailers from files.

## man page (slightly outdated) ##

<pre>
MP3CHECK(1)                                                        MP3CHECK(1)<br>
<br>
<br>
<br>
NAME<br>
mp3check - check mp3 files for consistency<br>
<br>
SYNOPSIS<br>
mp3check    [-03ABCEFGIKLMNPRSTWYZabcdefghlmopqrst]     [--accept=LIST]<br>
[--alt-color]   [--anomaly-check]   [--anomaly-check]   [--any-bitrate]<br>
[--any-crc]  [--any-emphasis]  [--any-layer]  [--any-mode]  [--any-sam-<br>
pling]  [--any-version]   [--ascii-only]   [--color]   [--compact-list]<br>
[--cut-junk-end]     [--cut-junk-start]    [--cut-tag-end]    [--dummy]<br>
[--dump-tag]   [--dump-header]    [--dump-tag]    [--edit-frame-byte=P]<br>
[--error-check]  [--error-check]  [--filelist=FILE] [--fix-crc] [--fix-<br>
headers]      [--help]      [--ign-bitrate-sw]      [--ign-constant-sw]<br>
[--ign-crc-error] [--ign-junk-end] [--ign-junk-start] [--ign-non-ampeg]<br>
[--ign-resync]  [--ign-tag128]   [--ign-truncated]   [--list]   [--log-<br>
file=FILE] [--max-errors=NUM] [--only-mp3] [--print-files] [--progress]<br>
[--quiet]   [--raw-elem-sep=NUM]   [--raw-line-sep=NUM]    [--raw-list]<br>
[--recursive]  [--reject=LIST]  [--show-valid]  [--single-line] [--ver-<br>
sion] [--xdev] [--] [FILES...]<br>
<br>
DESCRIPTION<br>
This manual page documents briefly the mp3check command.   This  manual<br>
page  was  written  for  the  Debian GNU/Linux distribution because the<br>
original program does not have a manual page.<br>
<br>
mp3check is a program that checks mp3 files for consistency and  prints<br>
several errors and warnings. It lists stream attributes (color).  Layer<br>
1,2,3, mpeg1.0+2.0 are currently supported.  CRC  check  for  layer  3.<br>
mp3check  is very useful for incomplete mp3 detection as it can be used<br>
to scan through your mp3 collection and find all mp3s that aren't  per-<br>
fect. Good for use with Napster and other bulk downloading of mp3s.<br>
<br>
OPTIONS<br>
These  programs  follow  the  usual  GNU command line syntax, with long<br>
options starting with two dashes (`-'). Options can be specified in any<br>
order  and  mixed with files. Option scanning stops after a double dash<br>
(--) to allow files beginning with a dash.  A  summary  of  options  is<br>
included below.<br>
<br>
mode:<br>
<br>
-l --list<br>
list parameters by examining the first valid header and size<br>
<br>
-c --compact-list<br>
list  parameters  of one file per line in a very compact format:<br>
version  (l=1.0,  L=2.0),  layer,   sampling   frequency   [kHz]<br>
(44=44.1),  bitrate  [kbit/s], mode (js=joint stereo, st=stereo,<br>
sc=single channel, dc=dual channel), emphasis  (n=none,  5=50/15<br>
usecs,  J=CCITT J.17), COY (has [C]rc, [O]riginal, cop[Y]right),<br>
length [min:sec], filename (poss. truncated)<br>
<br>
-e --error-check<br>
check crc and headers for consistency and  print  several  error<br>
messages<br>
<br>
-m --max-errors=<int><br>
with  -e:  set  maximum  number  of  errors  to  print  per file<br>
(0==infinity) (range=[0..])<br>
<br>
-a --anomaly-check<br>
report all differences from these parameters: layer 3,  44.1kHz,<br>
128kbps, joint stereo, no emphasis, has crc<br>
<br>
-d --dump-header<br>
dump all possible header with sync=0xfff<br>
<br>
-t --dump-tag<br>
dump all possible tags of known version<br>
<br>
--raw-list<br>
list  parameters in raw output format for use with external pro-<br>
grams<br>
<br>
--raw-elem-sep=NUM<br>
separate elements in one line by char NUM (numerical ASCII code)<br>
(default="0x09")<br>
<br>
--raw-line-sep=NUM<br>
separate    lines   by   char   NUM   (numerical   ASCII   code)<br>
(default="0x0a")<br>
<br>
--edit-frame-byte=P<br>
modify a single byte of a specific frame at a specific offset; B<br>
has  the format 'frame,offset,byteval', (use 0xff for hex or 255<br>
for dec or 0377 for octal); this  mode  operates  on  all  given<br>
files  and is useful for your own experiment with broken streams<br>
or while testing this toll ;-)<br>
<br>
fix errors:<br>
<br>
--cut-junk-start<br>
remove junk before first frame<br>
<br>
--cut-junk-end<br>
remove junk after last frame<br>
<br>
--fix-headers<br>
fix invalid  headers  (prevent  constant  parameter  switching),<br>
implies -e, use with care<br>
<br>
--fix-crc<br>
fix  crc  (set  crc to the calculated one), implies -e, use with<br>
care (note: it is not possible to add crc to  files  which  have<br>
been created without crc)<br>
<br>
disable error messages for -e --error-check:<br>
<br>
-G --ign-tag128<br>
ignore  128  byte  TAG  after  last frame -R --ign-resync ignore<br>
invalid frame header<br>
<br>
-E --ign-junk-end<br>
ignore junk after last frame<br>
<br>
-Z --ign-crc-error<br>
ignore crc errors<br>
<br>
-N --ign-non-ampeg<br>
ignore non audio mpeg streams<br>
<br>
-T --ign-truncated<br>
ignore truncated last frames<br>
<br>
-S --ign-junk-start<br>
ignore junk before first frame<br>
<br>
-B --ign-bitrate-sw<br>
ignore bitrate switching and enable VBR support<br>
<br>
-W --ign-constant-sw<br>
ignore switching of constant parameters, such as  sampling  fre-<br>
quency<br>
<br>
--show-valid<br>
print  the message 'valid audio mpeg stream' for all files which<br>
error free (after ignoring errors)<br>
<br>
disable anomaly messages for -a --anomaly-check:<br>
<br>
-C --any-crc<br>
ignore crc anomalies<br>
<br>
-M --any-mode<br>
ignore mode anomalies<br>
<br>
-L --any-layer<br>
ignore layer anomalies<br>
<br>
-K --any-bitrate<br>
ignore bitrate anomalies<br>
<br>
-I --any-version<br>
ignore version anomalies<br>
<br>
-F --any-sampling<br>
ignore sampling frequency anomalies<br>
<br>
-P --any-emphasis<br>
ignore emphasis anomalies<br>
<br>
file options:<br>
<br>
-r --recursive<br>
process any given directories recursively  (the  default  is  to<br>
ignore all directories specified on the command line)<br>
<br>
-f --filelist=FILE<br>
process  all  files specified in FILE (one filename per line) in<br>
addition to the command line<br>
<br>
-A --accept=LIST<br>
process only files with filename extensions specified  by  comma<br>
separated LIST<br>
<br>
-R --reject=LIST<br>
do  not  process  files  with  a filename extension specified by<br>
comma separated LIST<br>
<br>
-3 --only-mp3<br>
same as --accept mp3,MP3<br>
<br>
--xdev do not descend into other filesystems when recursing directories<br>
(doesn't work in Cygwin environment)<br>
<br>
--print-files<br>
just print all filenames without processing them, then exit<br>
<br>
output options:<br>
<br>
-s --single-line<br>
print  one  line  per file and message instead of splitting into<br>
several lines<br>
<br>
--no-summary<br>
suppress the summary printed  below  all  messages  if  multiple<br>
files are given<br>
<br>
-g --log-file=FILE<br>
print names of erroneous files to FILE, one per line<br>
<br>
-q --quiet<br>
quiet mode, hide messages about directories, non-regular or non-<br>
existing files<br>
<br>
-o --color<br>
colorize output with ANSI sequences<br>
<br>
-b --alt-color<br>
colorize: do not use bold ANSI sequences<br>
<br>
--ascii-only<br>
generally  all  unprintable  characters  in  filenames  etc  are<br>
replaced  by '!' (ASCII 0-31) and '?' (ASCII 127-159), with this<br>
option present the range ASCII 160-255 (which is usually  print-<br>
able: e.g. ISO-8859) is also printed as '?'<br>
<br>
-p --progress<br>
show progress information on stderr<br>
<br>
common options:<br>
<br>
-0 --dummy<br>
do not write/modify anything other than the logfile<br>
<br>
-h --help<br>
print this help message, then exit successfully<br>
<br>
--version<br>
print version, then exit successfully<br>
<br>
AUTHOR<br>
This original manual page was written by Klaus Kettner <kk@debian.org>,<br>
for the Debian GNU/Linux system. The current version of this manpage is<br>
maintained  by Johannes Overmann <Johannes.Overmann@gmx.de>, the author<br>
of mp3check.<br>
<br>
<br>
<br>
<br>
March  1, 2001                     MP3CHECK(1)<br>
</pre>