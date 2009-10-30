#!/usr/bin/perl -w

#my $confDisks = "/mnt/xenon01_*";
#my $confSelectedDisk = $ENV{"HOME"}."/.lazy_copy.dat";

#my $confCastor     = "/home/alphacdr/cdr";
my $confDcachePath = "/pnfs/triumf.ca/data/t2km11/aug2008";

$| = 1;

my $KiB = 1024;
my $MiB = 1024*1024;
my $GiB = 1024*1024*1024;

my $nsls = "/bin/ls -l";

my $file = shift @ARGV;
my @file = split(/\//, $file);
my $fname = pop @file;

die "Input file $file is not readable: $!\n" unless -r $file;

my $size = -s $file;

print "Consider $fname $size bytes $file\n";

my $isThere = checkFile($fname, $size);

if ($isThere)
  {
    # nothing to do
    print "File $fname already in dcache!\n";
    exit 0;
  }

my $dfile = $confDcachePath."/".$fname;

print "Backup $fname $size bytes $file to $dfile\n";

my $cmd = "/usr/bin/time dccp $file $dfile";
#my $cmd = "touch $dfile";
print "Run $cmd\n";
system $cmd;

die "Output file $dfile does not exist: $!\n" if ! -e $dfile;

my $dsize = -s $dfile;

die "Output file $dfile size mismatch: $dsize expected $size\n" unless $dsize == $size;

exit 0;

sub checkFile
  {
    my $file = shift @_;
    my $size = shift @_;

    my $dfile = "$confDcachePath/$file";

    my $cmd = "$nsls -l $dfile";
    my $ls = `$cmd 2>&1`;
    #print "[$ls]\n";

    return 0 if ($ls =~ /No such file or directory/);

    my ($perm, $nlink, $uid, $gid, $xsize) = split(/\s+/, $ls);
    #print "size [$size] [$xsize]\n";

    print "Found dcache file: $dfile, size: $size bytes\n";
# dCache will report a size of 1 is the file is > 2GB
    if ($size > 2147483647) {
       print "File is larger than 2 GB, dCache reports a size of $xsize";
       return 1 if ($xsize == 1);			    
       } else {
       return 1 if ($size == $xsize);
   }

    # file not found, or wrong size
    return 0;
  }

#end
