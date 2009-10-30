#!/usr/bin/perl -w

#my $confDisks = "/mnt/xenon01_*";
#my $confSelectedDisk = $ENV{"HOME"}."/.lazy_copy.dat";

#my $confCastorPath = "/castor/cern.ch/alpha/2007/data";
my $confCastorPath = "/castor/cern.ch/alpha/2009/data";

$| = 1;

my $KiB = 1024;
my $MiB = 1024*1024;
my $GiB = 1024*1024*1024;

my $nsls = "/usr/bin/nsls";

my $file = shift @ARGV;
my @file = split(/\//, $file);
my $fname = pop @file;

my $size = -s $file;

print "Backup $fname $size bytes $file\n";

open(MYOUTFILE, ">>/tmp/file.out");
print MYOUTFILE "Backup $fname $size bytes $file\n";

my $isThere = checkFile($fname, $size);
 
if ($isThere)
  {
    # nothing to do
    print "File $fname already in castor!\n";
    exit 0;
  }

my $dfile = $confCastorPath . "/" . $fname;

print "Backup $fname $size bytes $file to $dfile\n";

my $cmd = "ssh alphacdr\@alphacpc09 /usr/bin/time rfcp $file $dfile";
print "Run $cmd\n";
system $cmd;

my $check = checkFile($fname, $size);

if (!$check)
  {
    print "Cannot confirm that file was copied to castor!\n";
    exit 1;
  }

exit 0;

sub checkFile
  {
    my $file = shift @_;
    my $size = shift @_;

    my $cmd = "$nsls -l $confCastorPath/$file";
    my $ls = `$cmd 2>&1`;
    #print "[$ls]\n";

    return 0 if ($ls =~ /No such file or directory/);

    my ($perm, $nlink, $uid, $gid, $xsize) = split(/\s+/, $ls);
    #print "size [$size] [$xsize]\n";
    return 1 if ($size == $xsize);
    
    # file not found, or wrong size
    return 0;
  }

#end
